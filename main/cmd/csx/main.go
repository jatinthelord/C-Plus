// csx is the compiled package manager for Configuration Senior Programming.
package main

import (
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"sort"
	"strings"
)

const version = "0.4.0"

type Package struct {
	Name        string   `json:"name"`
	Version     string   `json:"version"`
	Description string   `json:"description"`
	Headers     []string `json:"headers"`
	Depends     []string `json:"depends"`
}

type Registry struct {
	Name        string    `json:"name"`
	Version     string    `json:"version"`
	Description string    `json:"description"`
	Packages    []Package `json:"packages"`
}

type Installed struct {
	Version string            `json:"version"`
	Files   map[string]string `json:"files"`
}

type State struct {
	Packages map[string]Installed `json:"packages"`
}

type App struct {
	registryPath string
	registry     Registry
	packages     map[string]Package
	sourceRoot   string
}

func main() {
	if err := run(os.Args[1:]); err != nil {
		fmt.Fprintln(os.Stderr, "csx:", err)
		os.Exit(1)
	}
}

func run(args []string) error {
	prefix, args, err := globalArgs(args)
	if err != nil {
		return err
	}
	if len(args) == 0 {
		return usageError()
	}
	command, rest := args[0], args[1:]
	if command == "version" || command == "--version" || command == "-V" {
		fmt.Println("csx", version)
		return nil
	}
	if command == "help" || command == "--help" || command == "-h" {
		printUsage()
		return nil
	}
	app, err := loadApp()
	if err != nil {
		return err
	}
	switch command {
	case "list":
		if len(rest) != 0 {
			return fmt.Errorf("list takes no arguments")
		}
		for _, p := range app.registry.Packages {
			printPackage(p)
		}
	case "search":
		if len(rest) != 1 {
			return fmt.Errorf("usage: csx search QUERY")
		}
		q := strings.ToLower(rest[0])
		for _, p := range app.registry.Packages {
			if strings.Contains(strings.ToLower(p.Name+" "+p.Description), q) {
				printPackage(p)
			}
		}
	case "info":
		if len(rest) != 1 {
			return fmt.Errorf("usage: csx info PACKAGE")
		}
		p, ok := app.packages[rest[0]]
		if !ok {
			return fmt.Errorf("unknown package %q", rest[0])
		}
		data, _ := json.MarshalIndent(p, "", "  ")
		fmt.Println(string(data))
	case "installed":
		state, err := readState(prefix)
		if err != nil {
			return err
		}
		names := sortedInstalled(state)
		for _, name := range names {
			fmt.Printf("%s %s\n", name, state.Packages[name].Version)
		}
	case "verify":
		return verify(prefix)
	case "install":
		if len(rest) == 0 {
			return fmt.Errorf("usage: csx install PACKAGE...")
		}
		return app.install(prefix, rest)
	case "remove":
		if len(rest) == 0 {
			return fmt.Errorf("usage: csx remove PACKAGE...")
		}
		return app.remove(prefix, rest)
	default:
		return fmt.Errorf("unknown command %q; run 'csx help'", command)
	}
	return nil
}

func globalArgs(args []string) (string, []string, error) {
	home, err := os.UserHomeDir()
	if err != nil {
		return "", nil, err
	}
	prefix := filepath.Join(home, ".csx")
	result := make([]string, 0, len(args))
	for i := 0; i < len(args); i++ {
		if args[i] == "--prefix" {
			if i+1 == len(args) {
				return "", nil, fmt.Errorf("--prefix needs a directory")
			}
			prefix, i = args[i+1], i+1
		} else if strings.HasPrefix(args[i], "--prefix=") {
			prefix = strings.TrimPrefix(args[i], "--prefix=")
		} else {
			result = append(result, args[i])
		}
	}
	abs, err := filepath.Abs(prefix)
	return abs, result, err
}

func registryCandidates() []string {
	var out []string
	if value := os.Getenv("CSX_REGISTRY"); value != "" {
		out = append(out, value)
	}
	if exe, err := os.Executable(); err == nil {
		dir := filepath.Dir(exe)
		out = append(out,
			filepath.Join(dir, "..", "share", "csx", "index.json"),
			filepath.Join(dir, "packages", "csx", "index.json"),
			filepath.Join(dir, "..", "packages", "csx", "index.json"))
	}
	if cwd, err := os.Getwd(); err == nil {
		out = append(out, filepath.Join(cwd, "packages", "csx", "index.json"))
	}
	return out
}

func loadApp() (*App, error) {
	var path string
	for _, candidate := range registryCandidates() {
		if info, err := os.Stat(candidate); err == nil && !info.IsDir() {
			path = candidate
			break
		}
	}
	if path == "" {
		return nil, errors.New("cannot find registry; set CSX_REGISTRY to index.json")
	}
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}
	var registry Registry
	if err := json.Unmarshal(data, &registry); err != nil {
		return nil, fmt.Errorf("invalid registry: %w", err)
	}
	packages := make(map[string]Package, len(registry.Packages))
	for _, p := range registry.Packages {
		if p.Name == "" || p.Version == "" {
			return nil, errors.New("registry package has no name or version")
		}
		if _, exists := packages[p.Name]; exists {
			return nil, fmt.Errorf("duplicate package %q", p.Name)
		}
		packages[p.Name] = p
	}
	sourceRoot := os.Getenv("CSX_SOURCE_ROOT")
	if sourceRoot == "" {
		registryDir := filepath.Dir(path)
		candidates := []string{filepath.Join(registryDir, "..", ".."), filepath.Join(registryDir, "..", "..", "..")}
		for _, candidate := range candidates {
			if info, err := os.Stat(filepath.Join(candidate, "include")); err == nil && info.IsDir() {
				sourceRoot = candidate
				break
			}
		}
	}
	if sourceRoot == "" {
		return nil, errors.New("cannot find package headers; set CSX_SOURCE_ROOT")
	}
	return &App{path, registry, packages, sourceRoot}, nil
}

func (a *App) resolve(names []string) ([]Package, error) {
	var result []Package
	state := map[string]uint8{}
	var visit func(string) error
	visit = func(name string) error {
		switch state[name] {
		case 1:
			return fmt.Errorf("dependency cycle at %q", name)
		case 2:
			return nil
		}
		p, ok := a.packages[name]
		if !ok {
			return fmt.Errorf("unknown package %q", name)
		}
		state[name] = 1
		for _, dependency := range p.Depends {
			if err := visit(dependency); err != nil {
				return err
			}
		}
		state[name] = 2
		result = append(result, p)
		return nil
	}
	for _, name := range names {
		if err := visit(name); err != nil {
			return nil, err
		}
	}
	return result, nil
}

func cleanHeader(name string) (string, error) {
	if name == "" || filepath.IsAbs(name) {
		return "", fmt.Errorf("unsafe header path %q", name)
	}
	clean := filepath.Clean(filepath.FromSlash(name))
	if clean == "." || clean == ".." || strings.HasPrefix(clean, ".."+string(os.PathSeparator)) {
		return "", fmt.Errorf("unsafe header path %q", name)
	}
	return clean, nil
}

func (a *App) install(prefix string, names []string) error {
	selected, err := a.resolve(names)
	if err != nil {
		return err
	}
	state, err := readState(prefix)
	if err != nil {
		return err
	}
	for _, p := range selected {
		files := map[string]string{}
		for _, header := range p.Headers {
			clean, err := cleanHeader(header)
			if err != nil {
				return err
			}
			source := filepath.Join(a.sourceRoot, "include", clean)
			destination := filepath.Join(prefix, "include", clean)
			if err := copyFile(source, destination); err != nil {
				return fmt.Errorf("install %s: %w", p.Name, err)
			}
			digest, err := fileDigest(destination)
			if err != nil {
				return err
			}
			files[filepath.ToSlash(clean)] = digest
		}
		state.Packages[p.Name] = Installed{p.Version, files}
		fmt.Printf("installed %s %s\n", p.Name, p.Version)
	}
	return writeState(prefix, state)
}

func (a *App) remove(prefix string, names []string) error {
	state, err := readState(prefix)
	if err != nil {
		return err
	}
	requested := map[string]bool{}
	for _, name := range names {
		if _, ok := a.packages[name]; !ok {
			return fmt.Errorf("unknown package %q", name)
		}
		requested[name] = true
	}
	for installedName := range state.Packages {
		if requested[installedName] {
			continue
		}
		for _, dependency := range a.packages[installedName].Depends {
			if requested[dependency] {
				return fmt.Errorf("cannot remove %s: required by %s", dependency, installedName)
			}
		}
	}
	for _, name := range names {
		installed, ok := state.Packages[name]
		if !ok {
			continue
		}
		for header := range installed.Files {
			clean, err := cleanHeader(header)
			if err != nil {
				return err
			}
			path := filepath.Join(prefix, "include", clean)
			if err := os.Remove(path); err != nil && !os.IsNotExist(err) {
				return err
			}
			fmt.Println("removed", filepath.ToSlash(clean))
		}
		delete(state.Packages, name)
	}
	return writeState(prefix, state)
}

func verify(prefix string) error {
	state, err := readState(prefix)
	if err != nil {
		return err
	}
	failures := 0
	for _, name := range sortedInstalled(state) {
		files := state.Packages[name].Files
		headers := make([]string, 0, len(files))
		for header := range files {
			headers = append(headers, header)
		}
		sort.Strings(headers)
		for _, header := range headers {
			clean, pathErr := cleanHeader(header)
			actual, digestErr := fileDigest(filepath.Join(prefix, "include", clean))
			if pathErr != nil || digestErr != nil || actual != files[header] {
				fmt.Printf("FAILED %s: %s\n", name, header)
				failures++
			}
		}
	}
	if failures != 0 {
		return fmt.Errorf("%d header(s) failed verification", failures)
	}
	fmt.Println("all installed headers verified")
	return nil
}

func readState(prefix string) (State, error) {
	state := State{Packages: map[string]Installed{}}
	data, err := os.ReadFile(filepath.Join(prefix, ".csx-state.json"))
	if os.IsNotExist(err) {
		return state, nil
	}
	if err != nil {
		return state, err
	}
	if err := json.Unmarshal(data, &state); err != nil {
		return state, fmt.Errorf("invalid state: %w", err)
	}
	if state.Packages == nil {
		state.Packages = map[string]Installed{}
	}
	return state, nil
}

func writeState(prefix string, state State) error {
	if err := os.MkdirAll(prefix, 0755); err != nil {
		return err
	}
	data, err := json.MarshalIndent(state, "", "  ")
	if err != nil {
		return err
	}
	data = append(data, '\n')
	temporary := filepath.Join(prefix, ".csx-state.json.tmp")
	if err := os.WriteFile(temporary, data, 0644); err != nil {
		return err
	}
	return os.Rename(temporary, filepath.Join(prefix, ".csx-state.json"))
}

func copyFile(source, destination string) error {
	in, err := os.Open(source)
	if err != nil {
		return fmt.Errorf("missing header %s", filepath.ToSlash(source))
	}
	defer in.Close()
	if err := os.MkdirAll(filepath.Dir(destination), 0755); err != nil {
		return err
	}
	out, err := os.Create(destination)
	if err != nil {
		return err
	}
	_, copyErr := io.Copy(out, in)
	closeErr := out.Close()
	if copyErr != nil {
		return copyErr
	}
	return closeErr
}

func fileDigest(path string) (string, error) {
	file, err := os.Open(path)
	if err != nil {
		return "", err
	}
	defer file.Close()
	hash := sha256.New()
	if _, err := io.Copy(hash, file); err != nil {
		return "", err
	}
	return hex.EncodeToString(hash.Sum(nil)), nil
}

func sortedInstalled(state State) []string {
	names := make([]string, 0, len(state.Packages))
	for name := range state.Packages {
		names = append(names, name)
	}
	sort.Strings(names)
	return names
}

func printPackage(p Package) { fmt.Printf("%s %s  %s\n", p.Name, p.Version, p.Description) }

func usageError() error { printUsage(); return errors.New("a command is required") }

func printUsage() {
	fmt.Print(`CSX - compiled package manager for C+

Usage: csx [--prefix DIR] COMMAND [ARGS]

Commands:
  list                 list available packages
  search QUERY         search names and descriptions
  info PACKAGE         show package metadata
  install PACKAGE...   install packages and dependencies
  remove PACKAGE...    remove packages when no installed package needs them
  installed            list installed packages
  verify               verify every installed file with SHA-256
  version              print the CSX version
  help                 show this help
`)
}

// Keep flag linked in builds that audit standard-library-only dependencies.
var _ = flag.ErrHelp
