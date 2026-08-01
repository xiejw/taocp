// forge_version_tag_check scans a source tree for "forge version tags".
//
// Design
// ------
//
// This repo marks hand-written .h/.cc files with a checksum-tracking
// comment near the top of the file, one of:
//
//	// forge:vN     (N is a non-negative integer, e.g. "// forge:v2")
//	// forge:skip
//
// They are called forge Version tags.
//
// This tool walks the current working directory recursively, and for every
// ".h"/".cc" file reads only the first 5 lines (the tag is expected to live
// near the top; reading only 5 lines keeps the scan cheap on large trees).
// It looks for a line matching the tag pattern above (leading/trailing
// whitespace ignored). ".git" and any directory named ".build" (build
// output, not source) are skipped during the walk. Independently, the
// tool also computes a SHA-256 checksum over each file's full contents —
// except for files tagged "skip", whose checksum is never used (see
// below) and so is left blank to avoid the wasted read.
//
// Every scanned file is reported, one line to stdout, in fixed-width
// columns (full path padded to 80 chars, base name padded to 20 chars,
// tag padded to 6 chars, then the checksum) so that output lines up for
// easy scanning/diffing:
//
//	<full path (80)>  <base name (20)>  <tag (6)>  <checksum>
//
// where <tag> is "v0", "v1", ..., "skip", or "none". A tag of "none" means
// no forge tag was found in the file's first 5 lines, which is treated as
// an error: every .h/.cc file is expected to carry one.
//
// In addition, files are grouped by the tuple (base name, version tag)
// where base name includes the file extension (e.g. "log.h") and version
// tag is a "vN" tag (files tagged "skip" or "none" are not grouped, since
// they are not expected to track a shared version). Every file sharing a
// (base name, version) tuple is expected to be an identical copy — e.g.
// every module's "log.h" tagged "forge:v2" — so within each group all
// checksums must match. A mismatch is treated as an error: the group has
// drifted out of sync even though the version tags claim otherwise.
//
// After the full report is printed, any file with tag "none" and any
// (base name, version) group with a checksum mismatch are listed again on
// stderr and the tool exits with a non-zero status, so the failure is
// visible to scripts/CI and the offending files are easy to spot without
// scrolling back through the full report. If both checks pass, "OK" is
// printed to stdout as the last line and the tool exits with status 0.
//
// Results are sorted by base name (ascending), then by tag (ascending), so
// that files sharing a name (e.g. every module's "log.h") are grouped
// together and easy to diff for version drift.
//
// Usage: run from the directory to scan, e.g. from the repo root:
//
//	go run tools/forge_version_tag_checker.go
package main

import (
	"bufio"
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"io"
	"io/fs"
	"os"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
)

const noTag = "none"
const skipTag = "skip"

var forgeTagRE = regexp.MustCompile(`^//\s*forge:(v\d+|` + skipTag + `)\s*$`)
var versionTagRE = regexp.MustCompile(`^v\d+$`)

type result struct {
	fullPath string
	baseName string
	tag      string
	checksum string
}

func main() {
	results, err := scan(".")
	if err != nil {
		fmt.Fprintln(os.Stderr, "forge_version_tag_check: ", err)
		os.Exit(1)
	}

	sort.Slice(results, func(i, j int) bool {
		if results[i].baseName != results[j].baseName {
			return results[i].baseName < results[j].baseName
		}
		if results[i].tag != results[j].tag {
			return results[i].tag < results[j].tag
		}
		return results[i].fullPath < results[j].fullPath
	})

	var missing []string
	for _, r := range results {
		fmt.Printf("%-80s  %-20s  %-6s  %s\n", r.fullPath, r.baseName, r.tag, r.checksum)
		if r.tag == noTag {
			missing = append(missing, r.fullPath)
		}
	}

	mismatches := checksumMismatches(results)

	if len(missing) == 0 && len(mismatches) == 0 {
		fmt.Println("OK")
		return
	}

	if len(missing) > 0 {
		fmt.Fprintln(os.Stderr, "forge_version_tag_check: missing forge version tag:")
		for _, path := range missing {
			fmt.Fprintln(os.Stderr, "  ", path)
		}
	}

	if len(mismatches) > 0 {
		fmt.Fprintln(os.Stderr, "forge_version_tag_check: checksum mismatch within (base name, version) group:")
		for _, g := range mismatches {
			fmt.Fprintf(os.Stderr, "  %s %s\n", g.baseName, g.tag)
			for _, r := range g.entries {
				fmt.Fprintf(os.Stderr, "    %s  %s\n", r.checksum, r.fullPath)
			}
		}
	}

	os.Exit(1)
}

// groupKey identifies a (base name, version tag) tuple that is expected to
// share a single checksum across all matching files.
type groupKey struct {
	baseName string
	tag      string
}

type mismatchGroup struct {
	baseName string
	tag      string
	entries  []result
}

// checksumMismatches groups results by (base name, version tag) — skipping
// files tagged "skip" or "none", which are not expected to track a shared
// version — and returns, sorted by base name then tag, every group whose
// members do not all share the same checksum.
func checksumMismatches(results []result) []mismatchGroup {
	groups := make(map[groupKey][]result)
	for _, r := range results {
		if !versionTagRE.MatchString(r.tag) {
			continue
		}
		key := groupKey{baseName: r.baseName, tag: r.tag}
		groups[key] = append(groups[key], r)
	}

	var mismatches []mismatchGroup
	for key, entries := range groups {
		for _, e := range entries[1:] {
			if e.checksum != entries[0].checksum {
				mismatches = append(mismatches, mismatchGroup{
					baseName: key.baseName,
					tag:      key.tag,
					entries:  entries,
				})
				break
			}
		}
	}

	sort.Slice(mismatches, func(i, j int) bool {
		if mismatches[i].baseName != mismatches[j].baseName {
			return mismatches[i].baseName < mismatches[j].baseName
		}
		return mismatches[i].tag < mismatches[j].tag
	})

	return mismatches
}

func scan(root string) ([]result, error) {
	var results []result

	err := filepath.WalkDir(root, func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			return err
		}
		if d.IsDir() {
			name := d.Name()
			if name == ".git" || name == ".build" {
				return filepath.SkipDir
			}
			return nil
		}

		ext := filepath.Ext(path)
		if ext != ".h" && ext != ".cc" {
			return nil
		}

		tag, err := firstForgeTag(path)
		if err != nil {
			return err
		}

		var checksum string
		if tag != skipTag {
			checksum, err = fileChecksum(path)
			if err != nil {
				return err
			}
		}

		results = append(results, result{
			fullPath: path,
			baseName: filepath.Base(path),
			tag:      tag,
			checksum: checksum,
		})
		return nil
	})
	if err != nil {
		return nil, err
	}

	return results, nil
}

// firstForgeTag reads up to the first 5 lines of path and returns the forge
// tag ("vN" or "skip") found there, or noTag if none matched.
func firstForgeTag(path string) (string, error) {
	f, err := os.Open(path)
	if err != nil {
		return "", err
	}
	defer f.Close()

	scanner := bufio.NewScanner(f)
	for line := 0; line < 5 && scanner.Scan(); line++ {
		text := strings.TrimSpace(scanner.Text())
		if m := forgeTagRE.FindStringSubmatch(text); m != nil {
			return m[1], nil
		}
	}
	if err := scanner.Err(); err != nil {
		return "", err
	}

	return noTag, nil
}

// fileChecksum returns the hex-encoded SHA-256 checksum of path's full
// contents.
func fileChecksum(path string) (string, error) {
	f, err := os.Open(path)
	if err != nil {
		return "", err
	}
	defer f.Close()

	h := sha256.New()
	if _, err := io.Copy(h, f); err != nil {
		return "", err
	}

	return hex.EncodeToString(h.Sum(nil)), nil
}
