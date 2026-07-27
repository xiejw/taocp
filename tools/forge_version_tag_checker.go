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
// output, not source) are skipped during the walk.
//
// Every scanned file is reported, one line to stdout, in fixed-width
// columns (full path padded to 80 chars, base name padded to 20 chars,
// then the tag) so that output lines up for easy scanning/diffing:
//
//	<full path (80)>  <base name (20)>  <tag>
//
// where <tag> is "v0", "v1", ..., "skip", or "none". A tag of "none" means
// no forge tag was found in the file's first 5 lines, which is treated as
// an error: every .h/.cc file is expected to carry one. After the full
// report is printed, if any file has tag "none", their full paths are
// listed again on stderr and the tool exits with a non-zero status, so the
// failure is visible to scripts/CI and the offending files are easy to spot
// without scrolling back through the full report.
//
// Results are sorted by base name (ascending), then by tag (ascending), so
// that files sharing a name (e.g. every module's "log.h") are grouped
// together and easy to diff for version drift.
//
// Usage: run from the directory to scan, e.g. from the repo root:
//
//	go run tools/forge_version_tag_check.go
package main

import (
	"bufio"
	"fmt"
	"io/fs"
	"os"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
)

const noTag = "none"

var forgeTagRE = regexp.MustCompile(`^//\s*forge:(v\d+|skip)\s*$`)

type result struct {
	fullPath string
	baseName string
	tag      string
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
		fmt.Printf("%-80s  %-20s  %s\n", r.fullPath, r.baseName, r.tag)
		if r.tag == noTag {
			missing = append(missing, r.fullPath)
		}
	}

	if len(missing) > 0 {
		fmt.Fprintln(os.Stderr, "forge_version_tag_check: missing forge version tag:")
		for _, path := range missing {
			fmt.Fprintln(os.Stderr, "  ", path)
		}
		os.Exit(1)
	}
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

		results = append(results, result{
			fullPath: path,
			baseName: filepath.Base(path),
			tag:      tag,
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
