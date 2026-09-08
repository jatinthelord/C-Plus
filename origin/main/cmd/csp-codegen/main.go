// csp-codegen validates and executes the deterministic compiler regression suite.
package main

import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
)

type testCase struct{ Name, File, Target, Kind string }
type manifest struct {
	Cases []testCase `json:"cases"`
}

func main() {
	validateOnly := flag.Bool("validate-only", false, "validate files without invoking the compiler")
	compileHost := flag.Bool("compile-host", false, "emit optimized assembly for host cases")
	limit := flag.Int("limit", 0, "limit checked cases; zero checks all")
	jobs := flag.Int("jobs", min(8, runtime.NumCPU()), "parallel compiler processes")
	shard := flag.String("shard", "", "one-based CI shard as INDEX/TOTAL")
	quiet := flag.Bool("quiet", false, "only print failures and final totals")
	compilerFlag := flag.String("compiler", "", "path to cspc")
	flag.Parse()
	if *jobs < 1 || *limit < 0 {
		fail("jobs must be positive and limit cannot be negative")
	}
	root, err := findRoot()
	if err != nil {
		fail(err.Error())
	}
	suite := filepath.Join(root, "tests", "codegen")
	data, err := os.ReadFile(filepath.Join(suite, "manifest.json"))
	if err != nil {
		fail(err.Error())
	}
	var payload manifest
	if err := json.Unmarshal(data, &payload); err != nil {
		fail("invalid manifest: " + err.Error())
	}
	if err := validate(suite, payload.Cases); err != nil {
		fail(err.Error())
	}
	folders := map[string]bool{}
	for _, item := range payload.Cases {
		parts := strings.Split(filepath.ToSlash(item.File), "/")
		if len(parts) > 2 {
			folders[parts[1]] = true
		}
	}
	if *validateOnly {
		fmt.Printf("validated %d CSP code-generation cases across %d case folders\n", len(payload.Cases), len(folders))
		return
	}
	cases := payload.Cases
	shardNote := ""
	if *shard != "" {
		parts := strings.Split(*shard, "/")
		if len(parts) != 2 {
			fail("shard must be INDEX/TOTAL")
		}
		index, e1 := strconv.Atoi(parts[0])
		total, e2 := strconv.Atoi(parts[1])
		if e1 != nil || e2 != nil || index < 1 || total < 1 || index > total {
			fail("shard must satisfy 1 <= INDEX <= TOTAL")
		}
		filtered := []testCase{}
		for i, item := range cases {
			if i%total == index-1 {
				filtered = append(filtered, item)
			}
		}
		cases = filtered
		shardNote = " (shard " + *shard + ")"
	}
	if *limit > 0 && *limit < len(cases) {
		cases = cases[:*limit]
	}
	compiler := *compilerFlag
	if compiler == "" {
		compiler = filepath.Join(root, "cspc")
		if runtime.GOOS == "windows" {
			compiler += ".exe"
		}
	}
	if _, err := os.Stat(compiler); err != nil {
		fail("missing compiler: " + compiler)
	}
	work := make(chan testCase)
	errors := make(chan error, 1)
	var completed atomic.Int64
	var wait sync.WaitGroup
	for worker := 0; worker < *jobs; worker++ {
		wait.Add(1)
		go func() {
			defer wait.Done()
			for item := range work {
				if err := check(root, suite, compiler, item, *compileHost); err != nil {
					select {
					case errors <- err:
					default:
					}
					continue
				}
				done := completed.Add(1)
				if !*quiet && (len(cases) <= 200 || done%100 == 0 || int(done) == len(cases)) {
					fmt.Printf("[%d/%d] checked\n", done, len(cases))
				}
			}
		}()
	}
	for _, item := range cases {
		work <- item
	}
	close(work)
	wait.Wait()
	close(errors)
	if err := <-errors; err != nil {
		fail(err.Error())
	}
	fmt.Printf("checked %d CSP code-generation cases with %d jobs%s\n", len(cases), *jobs, shardNote)
}

func findRoot() (string, error) {
	candidates := []string{}
	if cwd, err := os.Getwd(); err == nil {
		candidates = append(candidates, cwd)
	}
	if exe, err := os.Executable(); err == nil {
		dir := filepath.Dir(exe)
		candidates = append(candidates, dir, filepath.Dir(dir))
	}
	for _, root := range candidates {
		if info, err := os.Stat(filepath.Join(root, "tests", "codegen", "manifest.json")); err == nil && !info.IsDir() {
			return root, nil
		}
	}
	return "", fmt.Errorf("run from the CSP repository root")
}

func validate(suite string, cases []testCase) error {
	expected := map[string]bool{}
	for _, item := range cases {
		path, err := filepath.Abs(filepath.Join(suite, filepath.FromSlash(item.File)))
		if err != nil {
			return err
		}
		expected[path] = true
		data, err := os.ReadFile(path)
		if err != nil {
			return fmt.Errorf("missing %s", path)
		}
		marker := fmt.Sprintf("name=%s target=%s kind=%s", item.Name, item.Target, item.Kind)
		text := string(data)
		if !strings.Contains(text, marker) || !strings.Contains(text, "int main()") || strings.Count(text, "\n") < 7 {
			return fmt.Errorf("invalid or placeholder case: %s", path)
		}
	}
	err := filepath.WalkDir(filepath.Join(suite, "cases"), func(path string, entry os.DirEntry, walkErr error) error {
		if walkErr != nil {
			return walkErr
		}
		if !entry.IsDir() && strings.EqualFold(filepath.Ext(path), ".csp") {
			absolute, _ := filepath.Abs(path)
			if !expected[absolute] {
				return fmt.Errorf("unregistered case: %s", path)
			}
		}
		return nil
	})
	return err
}

func check(root, suite, compiler string, item testCase, compileHost bool) error {
	source := filepath.Join(suite, filepath.FromSlash(item.File))
	command := exec.Command(compiler, source, "--check")
	command.Dir = root
	var output bytes.Buffer
	command.Stdout = &output
	command.Stderr = &output
	if err := command.Run(); err != nil {
		return fmt.Errorf("FAILED %s\n%s", item.Name, output.String())
	}
	if compileHost && item.Target == "host" {
		path := filepath.Join(suite, "build", item.Name+".s")
		if err := os.MkdirAll(filepath.Dir(path), 0755); err != nil {
			return err
		}
		command = exec.Command(compiler, source, "--no-runtime", "-S", "-O3", "-o", path)
		command.Dir = root
		output.Reset()
		command.Stdout = &output
		command.Stderr = &output
		if err := command.Run(); err != nil {
			return fmt.Errorf("FAILED %s assembly emission\n%s", item.Name, output.String())
		}
		if info, err := os.Stat(path); err != nil || info.Size() == 0 {
			return fmt.Errorf("FAILED %s: empty assembly", item.Name)
		}
	}
	return nil
}

func fail(message string) { fmt.Fprintln(os.Stderr, "csp-codegen:", message); os.Exit(1) }
