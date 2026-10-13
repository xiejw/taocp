// menu is an interactive picker for the modules under src/.
//
// Design
// ------
//
// The tool walks "src" (relative to the current working directory)
// recursively and collects every sub directory that contains a Makefile,
// i.e., every directory where "make run" can be run. "src/mk" (build
// templates), ".git", ".build" and other hidden directories are skipped.
//
// The modules are shown in a selection box in the terminal:
//
//	Up/Down or k/j   move the cursor
//	Enter            run the command in the selected directory
//	ESC ESC          quit (also q or Ctrl-C)
//
// After the command finishes, the tool waits for Enter to go back to the
// selection box (or ESC twice to quit).
//
// The command defaults to "make run" and can be changed with -cmd, e.g.
//
//	go run tools/menu.go -cmd "make ASAN=1 test"
//
// No third-party dependency is used. The terminal is switched into raw
// mode via "stty", so this works on Linux and macOS.
package main

import (
	"flag"
	"fmt"
	"io/fs"
	"os"
	"os/exec"
	"path/filepath"
	"sort"
	"strings"
)

const (
	keyNone = iota
	keyUp
	keyDown
	keyEnter
	keyEsc
	keyQuit
)

// === --- main ------------------------------------------------------------ ===

var flagCmd = flag.String("cmd", "make run", "command to run in the selected directory")

func main() {
	flag.Parse()

	dirs, err := findDirs("src")
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to scan src: %v\n", err)
		os.Exit(1)
	}
	if len(dirs) == 0 {
		fmt.Fprintln(os.Stderr, "no sub directory with Makefile found under src")
		os.Exit(1)
	}

	saved, err := sttyGet()
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to read terminal state: %v\n", err)
		os.Exit(1)
	}
	defer sttySet(saved)

	cursor := 0
	for {
		idx, ok := selectDir(dirs, cursor)
		if !ok {
			break
		}
		cursor = idx
		runIn(dirs[idx], saved)
		if !waitForEnter() {
			break
		}
	}
	fmt.Print("\x1b[?25h") // \x1b[?25h: show cursor
}

// === --- System Utilities ------------------------------------------------ ===

// findDirs returns all sub directories of root holding a Makefile, sorted.
func findDirs(root string) ([]string, error) {
	var dirs []string
	skip := filepath.Join(root, "mk")
	err := filepath.WalkDir(root, func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			return err
		}
		if !d.IsDir() {
			return nil
		}
		if path == skip || (path != root && strings.HasPrefix(d.Name(), ".")) {
			return filepath.SkipDir
		}
		if path == root {
			return nil
		}
		if _, err := os.Stat(filepath.Join(path, "Makefile")); err == nil {
			dirs = append(dirs, path)
		}
		return nil
	})
	sort.Strings(dirs)
	return dirs, err
}

// runIn runs the command in dir with the terminal in its normal mode.
func runIn(dir, saved string) {
	sttySet(saved)
	fmt.Print("\x1b[H\x1b[2J")                                 // \x1b[H: cursor to top-left; \x1b[2J: clear screen
	fmt.Printf("\x1b[1;35m▶ %s: %s\x1b[0m\n\n", dir, *flagCmd) // \x1b[1;35m: bold magenta; \x1b[0m: reset attributes

	cmd := exec.Command("sh", "-c", *flagCmd)
	cmd.Dir = dir
	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		fmt.Printf("\n\x1b[1;31m✗ %v\x1b[0m\n", err) // \x1b[1;31m: bold red; \x1b[0m: reset attributes
	} else {
		fmt.Printf("\n\x1b[1;32m✓ done\x1b[0m\n") // \x1b[1;32m: bold green; \x1b[0m: reset attributes
	}
}

// === --- Drawing Utilities ----------------------------------------------- ===

// selectDir draws the selection box and returns the chosen index. ok is
// false if the user quits.
func selectDir(dirs []string, cursor int) (idx int, ok bool) {
	sttyRaw()
	fmt.Print("\x1b[?25l")       // \x1b[?25l: hide cursor
	defer fmt.Print("\x1b[?25h") // \x1b[?25h: show cursor

	top := 0
	escPending := false
	for {
		height := termHeight() - 6
		if height < 3 {
			height = 3
		}
		if height > len(dirs) {
			height = len(dirs)
		}
		if cursor < top {
			top = cursor
		}
		if cursor >= top+height {
			top = cursor - height + 1
		}
		draw(dirs, cursor, top, height, escPending)

		k := readKey()
		if k != keyEsc {
			escPending = false
		}
		switch k {
		case keyUp:
			cursor = (cursor - 1 + len(dirs)) % len(dirs)
		case keyDown:
			cursor = (cursor + 1) % len(dirs)
		case keyEnter:
			return cursor, true
		case keyQuit:
			return cursor, false
		case keyEsc:
			if escPending {
				return cursor, false
			}
			escPending = true
		}
	}
}

func draw(dirs []string, cursor, top, height int, escPending bool) {
	width := 0
	for _, d := range dirs {
		if len(d) > width {
			width = len(d)
		}
	}
	width += 4

	var b strings.Builder
	b.WriteString("\x1b[H\x1b[2J")                                           // \x1b[H: cursor to top-left; \x1b[2J: clear screen
	b.WriteString(fmt.Sprintf("Select a module to run `%s`:\r\n", *flagCmd)) // \r\n: raw mode needs an explicit carriage return
	b.WriteString("┌" + strings.Repeat("─", width) + "┐\r\n")
	for i := top; i < top+height; i++ {
		line := fmt.Sprintf("  %-*s", width-2, dirs[i])
		if i == cursor {
			line = "\x1b[7m> " + line[2:] + "\x1b[0m" // \x1b[7m: reverse video (highlight); \x1b[0m: reset attributes
		}
		b.WriteString("│" + line + "│\r\n")
	}
	b.WriteString("└" + strings.Repeat("─", width) + "┘\r\n")
	b.WriteString(fmt.Sprintf("[%d/%d]  ↑/↓ move  Enter run  ESC ESC quit", cursor+1, len(dirs)))
	if escPending {
		b.WriteString("  \x1b[33m(press ESC again to quit)\x1b[0m") // \x1b[33m: yellow text; \x1b[0m: reset attributes
	}
	b.WriteString("\r\n")
	fmt.Print(b.String())
}

// waitForEnter returns true on Enter and false on ESC twice (or q/Ctrl-C).
func waitForEnter() bool {
	fmt.Print("Press Enter to select again, ESC twice to quit.\r\n")
	sttyRaw()
	escPending := false
	for {
		switch readKey() {
		case keyEnter:
			return true
		case keyQuit:
			return false
		case keyEsc:
			if escPending {
				return false
			}
			escPending = true
		default:
			escPending = false
		}
	}
}

// readKey reads one key press in raw mode. A lone ESC byte is reported as
// keyEsc; escape sequences (arrow keys) arrive in the same read.
func readKey() int {
	buf := make([]byte, 16)
	n, err := os.Stdin.Read(buf)
	if err != nil || n == 0 {
		return keyQuit
	}
	s := string(buf[:n])
	switch s {
	case "\x1b": // lone ESC key
		return keyEsc
	case "\x1b[A", "\x1bOA", "k": // Up arrow (normal / application cursor mode), or vi-style k
		return keyUp
	case "\x1b[B", "\x1bOB", "j": // Down arrow (normal / application cursor mode), or vi-style j
		return keyDown
	case "\r", "\n": // Enter: CR in raw mode, LF as a fallback
		return keyEnter
	case "q", "\x03": // q, or Ctrl-C (\x03; raw mode disables SIGINT)
		return keyQuit
	}
	return keyNone
}

func stty(args ...string) (string, error) {
	cmd := exec.Command("stty", args...)
	cmd.Stdin = os.Stdin
	out, err := cmd.Output()
	return strings.TrimSpace(string(out)), err
}

func sttyGet() (string, error) { return stty("-g") }
func sttySet(state string)     { stty(state) }
func sttyRaw()                 { stty("raw", "-echo") }

func termHeight() int {
	out, err := stty("size")
	if err != nil {
		return 24
	}
	var rows, cols int
	if _, err := fmt.Sscanf(out, "%d %d", &rows, &cols); err != nil || rows == 0 {
		return 24
	}
	return rows
}
