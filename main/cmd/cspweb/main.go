// cspweb serves the CSP Foundation site and its local read-only source APIs.
package main

import (
	"encoding/json"
	"fmt"
	"io"
	"mime"
	"net/http"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"sync"
	"time"
)

type server struct {
	root, web string
	posts     sync.Mutex
	static    http.Handler
}

func main() {
	root, err := findRoot()
	if err != nil {
		panic(err)
	}
	port := os.Getenv("CSP_PORT")
	if port == "" {
		port = "8000"
	}
	s := &server{root: root, web: filepath.Join(root, "website")}
	s.static = http.FileServer(http.Dir(s.web))
	address := "127.0.0.1:" + port
	fmt.Println("CSP website: http://" + address)
	if err := http.ListenAndServe(address, s); err != nil {
		panic(err)
	}
}

func findRoot() (string, error) {
	var candidates []string
	if cwd, err := os.Getwd(); err == nil {
		candidates = append(candidates, cwd)
	}
	if exe, err := os.Executable(); err == nil {
		dir := filepath.Dir(exe)
		candidates = append(candidates, dir, filepath.Dir(dir))
	}
	for _, root := range candidates {
		if info, err := os.Stat(filepath.Join(root, "website", "index.html")); err == nil && !info.IsDir() {
			return root, nil
		}
	}
	return "", fmt.Errorf("run cspweb from the CSP repository root")
}

func (s *server) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodPost && r.URL.Path == "/api/wiki/posts" {
		s.createPost(w, r)
		return
	}
	if r.Method != http.MethodGet {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	switch r.URL.Path {
	case "/api/packages":
		s.fileJSON(w, filepath.Join(s.root, "packages", "csx", "index.json"))
		return
	case "/api/benchmarks":
		s.fileJSON(w, filepath.Join(s.root, "benchmarks", "results.json"))
		return
	case "/api/wiki/posts":
		s.wikiPosts(w)
		return
	case "/api/libraries":
		s.libraries(w)
		return
	case "/api/source":
		s.source(w, r)
		return
	case "/README.md", "/LICENSE", "/syntax.md":
		s.sendFile(w, filepath.Join(s.root, strings.TrimPrefix(r.URL.Path, "/")), "text/plain; charset=utf-8")
		return
	case "/include", "/include/":
		http.Redirect(w, r, "/source/", http.StatusFound)
		return
	case "/api/server-error":
		s.errorPage(w, 500, "server-error.html")
		return
	case "/api/redirect":
		target := r.URL.Query().Get("to")
		if target == "" || !strings.HasPrefix(target, "/") || strings.HasPrefix(target, "//") {
			s.errorPage(w, 400, "redirect-failed.html")
		} else {
			http.Redirect(w, r, target, http.StatusFound)
		}
		return
	}
	if r.URL.Path == "/packages/package.html" && !s.packageExists(r.URL.Query().Get("name")) {
		s.errorPage(w, 404, "package-not-found.html")
		return
	}
	s.static.ServeHTTP(w, r)
}

func (s *server) fileJSON(w http.ResponseWriter, path string) {
	data, err := os.ReadFile(path)
	if err != nil {
		jsonError(w, "data unavailable", 404)
		return
	}
	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.Write(data)
}
func (s *server) wikiPosts(w http.ResponseWriter) {
	var posts []map[string]any
	if err := readJSON(filepath.Join(s.web, "wiki", "posts.json"), &posts); err != nil {
		jsonError(w, "wiki post store is invalid", 500)
		return
	}
	sendJSON(w, map[string]any{"posts": posts}, 200)
}

func (s *server) libraries(w http.ResponseWriter) {
	var files []string
	for _, name := range []string{"include", "src"} {
		base := filepath.Join(s.root, name)
		filepath.WalkDir(base, func(path string, entry os.DirEntry, err error) error {
			if err == nil && !entry.IsDir() {
				relative, _ := filepath.Rel(s.root, path)
				files = append(files, filepath.ToSlash(relative))
			}
			return nil
		})
	}
	sort.Strings(files)
	sendJSON(w, map[string]any{"headers": files}, 200)
}

func (s *server) source(w http.ResponseWriter, r *http.Request) {
	requested := filepath.Clean(filepath.FromSlash(r.URL.Query().Get("file")))
	parts := strings.SplitN(requested, string(os.PathSeparator), 2)
	if len(parts) != 2 || (parts[0] != "src" && parts[0] != "include") {
		jsonError(w, "source file not found", 404)
		return
	}
	base, _ := filepath.Abs(filepath.Join(s.root, parts[0]))
	candidate, _ := filepath.Abs(filepath.Join(base, parts[1]))
	relative, err := filepath.Rel(base, candidate)
	if err != nil || relative == ".." || strings.HasPrefix(relative, ".."+string(os.PathSeparator)) {
		jsonError(w, "source file not found", 404)
		return
	}
	data, err := os.ReadFile(candidate)
	if err != nil {
		jsonError(w, "source file not found", 404)
		return
	}
	sendJSON(w, map[string]any{"file": filepath.ToSlash(requested), "content": string(data)}, 200)
}

func (s *server) createPost(w http.ResponseWriter, r *http.Request) {
	defer r.Body.Close()
	body, err := io.ReadAll(io.LimitReader(r.Body, 7000))
	if err != nil {
		jsonError(w, "invalid request", 400)
		return
	}
	var input map[string]string
	if json.Unmarshal(body, &input) != nil {
		jsonError(w, "invalid JSON", 400)
		return
	}
	title, author, content := strings.TrimSpace(input["title"]), strings.TrimSpace(input["author"]), strings.TrimSpace(input["body"])
	category := strings.TrimSpace(input["category"])
	if category == "" {
		category = "Discussion"
	}
	if title == "" || author == "" || content == "" || len(title) > 120 || len(author) > 80 || len(content) > 5000 {
		jsonError(w, "title, author, and valid post content are required", 400)
		return
	}
	post := map[string]string{"title": title, "author": author, "category": category, "body": content, "created": time.Now().UTC().Format(time.RFC3339)}
	s.posts.Lock()
	defer s.posts.Unlock()
	path := filepath.Join(s.web, "wiki", "posts.json")
	var posts []map[string]any
	if readJSON(path, &posts) != nil {
		jsonError(w, "wiki post store is invalid", 500)
		return
	}
	posts = append([]map[string]any{toAny(post)}, posts...)
	data, _ := json.MarshalIndent(posts, "", "  ")
	data = append(data, '\n')
	temporary := path + ".tmp"
	if os.WriteFile(temporary, data, 0644) != nil || os.Rename(temporary, path) != nil {
		jsonError(w, "cannot save post", 500)
		return
	}
	sendJSON(w, map[string]any{"post": post}, 201)
}

func (s *server) packageExists(name string) bool {
	if name == "" {
		return false
	}
	var value struct {
		Packages []struct {
			Name string `json:"name"`
		} `json:"packages"`
	}
	if readJSON(filepath.Join(s.root, "packages", "csx", "index.json"), &value) != nil {
		return false
	}
	for _, item := range value.Packages {
		if item.Name == name {
			return true
		}
	}
	return false
}
func (s *server) errorPage(w http.ResponseWriter, status int, name string) {
	path := filepath.Join(s.web, "errors", name)
	data, err := os.ReadFile(path)
	if err != nil {
		http.Error(w, http.StatusText(status), status)
		return
	}
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	w.WriteHeader(status)
	w.Write(data)
}
func (s *server) sendFile(w http.ResponseWriter, path, contentType string) {
	data, err := os.ReadFile(path)
	if err != nil {
		http.NotFound(w, nil)
		return
	}
	if contentType == "" {
		contentType = mime.TypeByExtension(filepath.Ext(path))
	}
	w.Header().Set("Content-Type", contentType)
	w.Write(data)
}
func readJSON(path string, target any) error {
	data, err := os.ReadFile(path)
	if err != nil {
		return err
	}
	data = bytesTrimBOM(data)
	return json.Unmarshal(data, target)
}
func bytesTrimBOM(data []byte) []byte {
	if len(data) >= 3 && data[0] == 0xef && data[1] == 0xbb && data[2] == 0xbf {
		return data[3:]
	}
	return data
}
func sendJSON(w http.ResponseWriter, value any, status int) {
	data, _ := json.MarshalIndent(value, "", "  ")
	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.WriteHeader(status)
	w.Write(data)
}
func jsonError(w http.ResponseWriter, message string, status int) {
	sendJSON(w, map[string]string{"error": message}, status)
}
func toAny(value map[string]string) map[string]any {
	result := map[string]any{}
	for key, item := range value {
		result[key] = item
	}
	return result
}
