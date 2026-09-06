// csp-bench builds and measures equivalent C+, C, C++, and Rust workloads.
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
	"sort"
	"strconv"
	"strings"
	"time"
)

type result struct {
	Language    string    `json:"language"`
	Status      string    `json:"status"`
	Compiler    string    `json:"compiler,omitempty"`
	CompileMS   float64   `json:"compile_ms,omitempty"`
	MedianMS    float64   `json:"median_ms,omitempty"`
	MinMS       float64   `json:"min_ms,omitempty"`
	MaxMS       float64   `json:"max_ms,omitempty"`
	BinaryBytes int64     `json:"binary_bytes,omitempty"`
	Checksum    string    `json:"checksum,omitempty"`
	Samples     []float64 `json:"samples_ms,omitempty"`
	Command     []string  `json:"command,omitempty"`
	Error       string    `json:"error,omitempty"`
}
type candidate struct {
	language, compiler string
	command            []string
	output             string
}

func main() {
	runs := flag.Int("runs", 7, "timed samples per language")
	warmup := flag.Int("warmup", 2, "untimed warm-up runs")
	rounds := flag.Int("rounds", 25000000, "workload iterations")
	noWeb := flag.Bool("no-web", false, "do not refresh website data")
	native := flag.Bool("native", false, "tune every native compiler for this CPU")
	flag.Parse()
	if *runs < 1 || *warmup < 0 || *rounds < 1 {
		fail("runs and rounds must be positive; warmup cannot be negative")
	}
	root, err := findRoot()
	if err != nil {
		fail(err.Error())
	}
	here := filepath.Join(root, "benchmarks")
	build := filepath.Join(here, "build")
	os.MkdirAll(build, 0755)
	ext := ""
	if runtime.GOOS == "windows" {
		ext = ".exe"
	}
	executable := func(name string) string { return filepath.Join(build, name+ext) }
	// Prefer the fresh CMake output over a possibly older convenience copy in
	// the repository root.
	cspc := firstExisting(filepath.Join(root, "build", "cspc"+ext), filepath.Join(root, "cspc"+ext))
	cc := firstTool("gcc", "clang")
	cxx := firstTool("g++", "clang++")
	rustc := firstTool("rustc")
	cFlags := []string{"-std=c17", "-O3"}
	cxxFlags := []string{"-std=c++20", "-O3"}
	cspFlags := []string{filepath.Join(here, "workload.csp"), "--no-runtime", "--compiler", cxx, "-O3", "--rebuild"}
	rustFlags := []string{"-C", "opt-level=3"}
	if *native {
		// Apply equivalent CPU tuning to every implementation. This avoids
		// manufacturing a C+ win by giving only C+ stronger backend flags.
		cFlags = append(cFlags, "-march=native", "-flto")
		cxxFlags = append(cxxFlags, "-march=native", "-flto")
		cspFlags = append(cspFlags, "--native", "--lto")
		rustFlags = append(rustFlags, "-C", "target-cpu=native", "-C", "lto=yes")
	}
	candidates := []candidate{
		{"C+", existingCompiler(cspc, cxx), append(append([]string{cspc}, cspFlags...), "-o", executable("workload-csp")), executable("workload-csp")},
		{"C", cc, append(append([]string{cc}, cFlags...), filepath.Join(here, "workload.c"), "-o", executable("workload-c")), executable("workload-c")},
		{"C++", cxx, append(append([]string{cxx}, cxxFlags...), filepath.Join(here, "workload.cpp"), "-o", executable("workload-cpp")), executable("workload-cpp")},
		{"Rust", rustc, append(append([]string{rustc}, rustFlags...), filepath.Join(here, "workload.rs"), "-o", executable("workload-rust")), executable("workload-rust")},
	}
	if runtime.GOOS == "windows" && runtime.GOARCH == "amd64" {
		asmCommand := []string{cc, filepath.Join(here, "workload_windows_x86_64.s"), "-o", executable("workload-asm")}
		candidates = append(candidates, candidate{"ASM x86-64", cc, asmCommand, executable("workload-asm")})
	}
	results := []result{}
	expected := ""
	for _, item := range candidates {
		if item.compiler == "" {
			results = append(results, result{Language: item.language, Status: "not installed"})
			continue
		}
		started := time.Now()
		output, err := run(item.command, root)
		compileMS := milliseconds(time.Since(started))
		if err != nil {
			results = append(results, result{Language: item.language, Status: "build failed", Error: tail(output, 2000)})
			continue
		}
		arguments := []string{item.output, strconv.Itoa(*rounds)}
		okay := true
		for i := 0; i < *warmup; i++ {
			if _, err := run(arguments, root); err != nil {
				okay = false
				break
			}
		}
		if !okay {
			results = append(results, result{Language: item.language, Status: "run failed"})
			continue
		}
		samples := make([]float64, 0, *runs)
		checksum := ""
		for i := 0; i < *runs; i++ {
			started = time.Now()
			value, runErr := run(arguments, root)
			samples = append(samples, milliseconds(time.Since(started)))
			if runErr != nil {
				okay = false
				output = value
				break
			}
			checksum = strings.TrimSpace(value)
		}
		if !okay {
			results = append(results, result{Language: item.language, Status: "run failed", Error: tail(output, 2000)})
			continue
		}
		if expected == "" {
			expected = checksum
		}
		status := "ok"
		if checksum != expected {
			status = "checksum mismatch"
		}
		ordered := append([]float64(nil), samples...)
		sort.Float64s(ordered)
		median := ordered[len(ordered)/2]
		if len(ordered)%2 == 0 {
			median = (ordered[len(ordered)/2-1] + ordered[len(ordered)/2]) / 2
		}
		info, _ := os.Stat(item.output)
		compilerVersion := version(item.compiler)
		if item.language == "C+" {
			compilerVersion = version(cspc) + " via " + version(cxx)
		}
		results = append(results, result{Language: item.language, Status: status, Compiler: compilerVersion, CompileMS: round(compileMS), MedianMS: round(median), MinMS: round(ordered[0]), MaxMS: round(ordered[len(ordered)-1]), BinaryBytes: info.Size(), Checksum: checksum, Samples: roundAll(samples), Command: item.command})
	}
	optimization := "release / level 3"
	if *native {
		optimization += " / native CPU / LTO"
	}
	payload := map[string]any{"generated_at": time.Now().UTC().Format(time.RFC3339), "machine": map[string]string{"os": runtime.GOOS, "architecture": runtime.GOARCH, "cpu": os.Getenv("PROCESSOR_IDENTIFIER"), "runner": "Go " + runtime.Version()}, "method": map[string]any{"workload": "xorshift64* integer mixing", "rounds": *rounds, "runs": *runs, "warmup": *warmup, "optimization": optimization, "timer": "wall clock including process startup"}, "results": results}
	data, _ := json.MarshalIndent(payload, "", "  ")
	data = append(data, '\n')
	jsonPath := filepath.Join(here, "results.json")
	os.WriteFile(jsonPath, data, 0644)
	if !*noWeb {
		asset := filepath.Join(root, "website", "assets", "benchmarks.js")
		os.WriteFile(asset, append([]byte("window.CSP_BENCHMARK_DATA = "), append(data, []byte(";\n")...)...), 0644)
	}
	fmt.Printf("%-9s %15s %10s %10s %11s %11s\n", "Language", "Runtime median", "min", "max", "compile", "bytes")
	failed := false
	for _, item := range results {
		if item.Status != "ok" {
			fmt.Printf("%-9s %15s\n", item.Language, item.Status)
			if item.Status != "not installed" {
				failed = true
			}
		} else {
			fmt.Printf("%-9s %12.3f ms %7.3f ms %7.3f ms %8.3f ms %11d\n", item.Language, item.MedianMS, item.MinMS, item.MaxMS, item.CompileMS, item.BinaryBytes)
		}
	}
	fmt.Println("\nRaw data:", jsonPath)
	if failed {
		os.Exit(1)
	}
}

func findRoot() (string, error) {
	candidates := []string{}
	if cwd, e := os.Getwd(); e == nil {
		candidates = append(candidates, cwd)
	}
	if exe, e := os.Executable(); e == nil {
		d := filepath.Dir(exe)
		candidates = append(candidates, d, filepath.Dir(d))
	}
	for _, r := range candidates {
		if _, e := os.Stat(filepath.Join(r, "benchmarks", "workload.csp")); e == nil {
			return r, nil
		}
	}
	return "", fmt.Errorf("run from the CSP repository root")
}
func firstTool(names ...string) string {
	for _, name := range names {
		if path, e := exec.LookPath(name); e == nil {
			return path
		}
	}
	return ""
}
func firstExisting(paths ...string) string {
	for _, path := range paths {
		if _, err := os.Stat(path); err == nil {
			return path
		}
	}
	return ""
}
func existingCompiler(cspc, cxx string) string {
	if cxx == "" {
		return ""
	}
	if _, e := os.Stat(cspc); e == nil {
		return cspc
	}
	return ""
}
func run(arguments []string, directory string) (string, error) {
	if len(arguments) == 0 || arguments[0] == "" {
		return "", fmt.Errorf("missing command")
	}
	command := exec.Command(arguments[0], arguments[1:]...)
	command.Dir = directory
	var output bytes.Buffer
	command.Stdout = &output
	command.Stderr = &output
	err := command.Run()
	return output.String(), err
}
func version(command string) string {
	output, _ := run([]string{command, "--version"}, "")
	lines := strings.Split(strings.TrimSpace(output), "\n")
	if len(lines) > 0 {
		return strings.TrimSpace(lines[0])
	}
	return "unknown version"
}
func milliseconds(value time.Duration) float64 { return float64(value.Nanoseconds()) / 1e6 }
func round(v float64) float64                  { x, _ := strconv.ParseFloat(fmt.Sprintf("%.3f", v), 64); return x }
func roundAll(values []float64) []float64 {
	out := make([]float64, len(values))
	for i, v := range values {
		out[i] = round(v)
	}
	return out
}
func tail(value string, size int) string {
	if len(value) > size {
		return value[len(value)-size:]
	}
	return value
}
func fail(message string) { fmt.Fprintln(os.Stderr, "csp-bench:", message); os.Exit(2) }
