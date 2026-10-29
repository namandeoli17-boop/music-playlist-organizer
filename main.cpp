/*
 * MUSIC PLAYLIST ORGANIZER  (A Data Structure Based C++ Application)
 * Subject : Data Structures using C++  | BCA 3rd Semester
 *
 * Data structures used (see Table 1 of the proposal):
 *   - Doubly circular linked list : song library & playlists (next/prev, repeat-all)
 *   - Queue (linked list)         : play queue ("up next")
 *   - Stack (linked list)         : recently played history + undo
 *   - Hash table (chaining)       : fast search by title / artist
 *   - Binary Search Tree          : alphabetical display
 *   - Merge sort + Insertion sort : sort by title / artist / duration
 *   - Array + Fisher-Yates        : shuffle play
 *   - File handling               : save / load (playlists.txt)
 *   - HTML + CSS export           : index.html styled by style.css
 *
 * Compile:  g++ -std=c++11 -o playlist main.cpp
 */

#include <algorithm>
#include <chrono>
#include <cctype>   
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

/* ======================================================================
 *  HELPERS
 * ====================================================================== */
string toLower(string s) {
    for (size_t i = 0; i < s.size(); i++) s[i] = (char)tolower((unsigned char)s[i]);
    return s;
}

string fmtDuration(int sec) {
    ostringstream o;
    o << sec / 60 << ":" << setw(2) << setfill('0') << sec % 60;
    return o.str();
}

// '|' is our file delimiter, so it is not allowed inside text fields
string clean(string s) {
    for (size_t i = 0; i < s.size(); i++) if (s[i] == '|') s[i] = '/';
    return s;
}

string htmlEscape(const string &s) {
    string r;
    for (size_t i = 0; i < s.size(); i++) {
        switch (s[i]) {
            case '&': r += "&amp;";  break;
            case '<': r += "&lt;";   break;
            case '>': r += "&gt;";   break;
            case '"': r += "&quot;"; break;
            default:  r += s[i];
        }
    }
    return r;
}

string ask(const string &prompt) {
    cout << prompt;
    string s;
    if (!getline(cin, s)) { cout << "\nInput closed. Exiting.\n"; exit(0); }
    return s;
}

int askInt(const string &prompt, int lo, int hi) {
    while (true) {
        string s = ask(prompt);
        try {
            size_t pos;
            int v = stoi(s, &pos);
            if (pos == s.size() && v >= lo && v <= hi) return v;
        } catch (...) {}
        cout << "  Invalid input. Enter a number between " << lo << " and " << hi << ".\n";
    }
}

int askDuration() {  // accepts "mm:ss" or seconds
    while (true) {
        string s = ask("  Duration (mm:ss or seconds): ");
        try {
            size_t c = s.find(':');
            int total;
            if (c == string::npos) total = stoi(s);
            else total = stoi(s.substr(0, c)) * 60 + stoi(s.substr(c + 1));
            if (total > 0 && total < 36000) return total;
        } catch (...) {}
        cout << "  Invalid duration. Try again.\n";
    }
}

/* ======================================================================
 *  SONG
 * ====================================================================== */
struct Song {
    string title, artist, album, genre;
    int duration;  // seconds
    Song() : duration(0) {}
    Song(string t, string a, string al, string g, int d)
        : title(clean(t)), artist(clean(a)), album(clean(al)), genre(clean(g)), duration(d) {}
    bool sameAs(const Song &o) const {
        return toLower(title) == toLower(o.title) && toLower(artist) == toLower(o.artist);
    }
};

/* ======================================================================
 *  DOUBLY CIRCULAR LINKED LIST  (library & playlists)
 * ====================================================================== */
struct DNode {
    Song data;
    DNode *next, *prev;
    DNode(const Song &s) : data(s), next(this), prev(this) {}
};

class Playlist {
public:
    string name;
    DNode *head;
    DNode *cur;  // pointer used by next / previous
    int count;

    Playlist(const string &n = "") : name(n), head(nullptr), cur(nullptr), count(0) {}
    ~Playlist() { clear(); }
    Playlist(const Playlist &) = delete;
    Playlist &operator=(const Playlist &) = delete;

    void clear() {
        while (head) removeNode(head);
        cur = nullptr;
    }

    // O(1) insert at the end (tail = head->prev)
    void addBack(const Song &s) {
        DNode *n = new DNode(s);
        if (!head) {
            head = n;
        } else {
            DNode *tail = head->prev;
            n->prev = tail;
            n->next = head;
            tail->next = n;
            head->prev = n;
        }
        count++;
    }

    // O(1) delete at a known node
    void removeNode(DNode *n) {
        if (count == 1) {
            head = nullptr;
            cur = nullptr;
        } else {
            n->prev->next = n->next;
            n->next->prev = n->prev;
            if (head == n) head = n->next;
            if (cur == n) cur = n->prev;
        }
        delete n;
        count--;
    }

    DNode *findNode(const string &title) const {
        if (!head) return nullptr;
        DNode *p = head;
        string t = toLower(title);
        do {
            if (toLower(p->data.title) == t) return p;
            p = p->next;
        } while (p != head);
        return nullptr;
    }

    bool contains(const Song &s) const {
        if (!head) return false;
        DNode *p = head;
        do {
            if (p->data.sameAs(s)) return true;
            p = p->next;
        } while (p != head);
        return false;
    }

    bool removeByTitle(const string &title, Song *removed = nullptr) {
        DNode *n = findNode(title);
        if (!n) return false;
        if (removed) *removed = n->data;
        removeNode(n);
        return true;
    }

    // Replace every copy of 'old' with 'updated' (used by Edit Song)
    void updateMatching(const Song &old, const Song &updated) {
        if (!head) return;
        DNode *p = head;
        do {
            if (p->data.sameAs(old)) p->data = updated;
            p = p->next;
        } while (p != head);
    }

    // Circular navigation: repeat-all comes for free
    Song *next() {
        if (!head) return nullptr;
        cur = cur ? cur->next : head;
        return &cur->data;
    }
    Song *prev() {
        if (!head) return nullptr;
        cur = cur ? cur->prev : head->prev;
        return &cur->data;
    }

    vector<Song> toVector() const {
        vector<Song> v;
        if (!head) return v;
        DNode *p = head;
        do {
            v.push_back(p->data);
            p = p->next;
        } while (p != head);
        return v;
    }

    void rebuild(const vector<Song> &v) {
        clear();
        for (size_t i = 0; i < v.size(); i++) addBack(v[i]);
    }

    void display() const {
        if (!head) { cout << "  (empty)\n"; return; }
        cout << left << "  " << setw(4) << "No." << setw(26) << "Title" << setw(20) << "Artist"
             << setw(18) << "Album" << setw(11) << "Genre" << "Time\n";
        cout << "  " << string(84, '-') << "\n";
        DNode *p = head;
        int i = 1;
        do {
            const Song &s = p->data;
            cout << "  " << setw(4) << i++ << setw(26) << s.title.substr(0, 24) << setw(20)
                 << s.artist.substr(0, 18) << setw(18) << s.album.substr(0, 16) << setw(11)
                 << s.genre.substr(0, 9) << fmtDuration(s.duration) << "\n";
            p = p->next;
        } while (p != head);
    }
};

/* ======================================================================
 *  STACK (linked list)  - history and undo
 * ====================================================================== */
template <typename T>
class Stack {
    struct Node { T data; Node *next; Node(const T &d, Node *n) : data(d), next(n) {} };
    Node *topNode;
    int n;
public:
    Stack() : topNode(nullptr), n(0) {}
    ~Stack() { clear(); }
    Stack(const Stack &) = delete;
    Stack &operator=(const Stack &) = delete;

    void push(const T &x) { topNode = new Node(x, topNode); n++; }
    bool pop(T &out) {
        if (!topNode) return false;
        Node *t = topNode;
        out = t->data;
        topNode = t->next;
        delete t;
        n--;
        return true;
    }
    bool peek(T &out) const { if (!topNode) return false; out = topNode->data; return true; }
    bool empty() const { return topNode == nullptr; }
    int size() const { return n; }
    void clear() { T tmp; while (pop(tmp)) {} }
    vector<T> toVector() const {  // most recent first
        vector<T> v;
        for (Node *p = topNode; p; p = p->next) v.push_back(p->data);
        return v;
    }
};

/* ======================================================================
 *  QUEUE (linked list)  - play queue
 * ====================================================================== */
template <typename T>
class Queue {
    struct Node { T data; Node *next; Node(const T &d) : data(d), next(nullptr) {} };
    Node *front, *rear;
    int n;
public:
    Queue() : front(nullptr), rear(nullptr), n(0) {}
    ~Queue() { clear(); }
    Queue(const Queue &) = delete;
    Queue &operator=(const Queue &) = delete;

    void enqueue(const T &x) {
        Node *nd = new Node(x);
        if (rear) rear->next = nd; else front = nd;
        rear = nd;
        n++;
    }
    bool dequeue(T &out) {
        if (!front) return false;
        Node *t = front;
        out = t->data;
        front = t->next;
        if (!front) rear = nullptr;
        delete t;
        n--;
        return true;
    }
    bool empty() const { return front == nullptr; }
    int size() const { return n; }
    void clear() { T tmp; while (dequeue(tmp)) {} }
    vector<T> toVector() const {
        vector<T> v;
        for (Node *p = front; p; p = p->next) v.push_back(p->data);
        return v;
    }
};

/* ======================================================================
 *  HASH TABLE (separate chaining) - search by title / artist
 * ====================================================================== */
class HashTable {
    struct HNode { string key; Song song; HNode *next; HNode(const string &k, const Song &s, HNode *n) : key(k), song(s), next(n) {} };
    vector<HNode *> buckets;

    size_t hashKey(const string &k) const {
        unsigned long h = 5381;
        for (size_t i = 0; i < k.size(); i++) h = h * 33 + (unsigned char)k[i];
        return h % buckets.size();
    }
public:
    HashTable(size_t size = 101) : buckets(size, nullptr) {}
    ~HashTable() { clear(); }
    HashTable(const HashTable &) = delete;
    HashTable &operator=(const HashTable &) = delete;

    void insert(const string &key, const Song &s) {
        string k = toLower(key);
        size_t i = hashKey(k);
        buckets[i] = new HNode(k, s, buckets[i]);
    }
    vector<Song> search(const string &key) const {  // O(1) average
        vector<Song> res;
        string k = toLower(key);
        for (HNode *p = buckets[hashKey(k)]; p; p = p->next)
            if (p->key == k) res.push_back(p->song);
        return res;
    }
    void clear() {
        for (size_t i = 0; i < buckets.size(); i++) {
            HNode *p = buckets[i];
            while (p) { HNode *t = p; p = p->next; delete t; }
            buckets[i] = nullptr;
        }
    }
};

/* ======================================================================
 *  BINARY SEARCH TREE - alphabetical order
 * ====================================================================== */
class BST {
    struct TNode { Song s; TNode *l, *r; TNode(const Song &x) : s(x), l(nullptr), r(nullptr) {} };
    TNode *root;

    TNode *insert(TNode *t, const Song &s) {
        if (!t) return new TNode(s);
        if (toLower(s.title) < toLower(t->s.title)) t->l = insert(t->l, s);
        else t->r = insert(t->r, s);
        return t;
    }
    void inorder(TNode *t, vector<Song> &out) const {
        if (!t) return;
        inorder(t->l, out);
        out.push_back(t->s);
        inorder(t->r, out);
    }
    void destroy(TNode *t) { if (!t) return; destroy(t->l); destroy(t->r); delete t; }
public:
    BST() : root(nullptr) {}
    ~BST() { destroy(root); }
    BST(const BST &) = delete;
    BST &operator=(const BST &) = delete;

    void insert(const Song &s) { root = insert(root, s); }
    vector<Song> sorted() const { vector<Song> v; inorder(root, v); return v; }
    void clear() { destroy(root); root = nullptr; }
};

/* ======================================================================
 *  SORTING  (merge sort + insertion sort for small ranges)
 * ====================================================================== */
enum SortField { BY_TITLE = 1, BY_ARTIST = 2, BY_DURATION = 3 };

bool goesBefore(const Song &a, const Song &b, int field, bool asc) {
    int c = 0;
    if (field == BY_TITLE) c = toLower(a.title).compare(toLower(b.title));
    else if (field == BY_ARTIST) c = toLower(a.artist).compare(toLower(b.artist));
    else c = (a.duration < b.duration) ? -1 : (a.duration > b.duration ? 1 : 0);
    return asc ? c < 0 : c > 0;
}

void insertionSort(vector<Song> &v, int l, int r, int field, bool asc) {
    for (int i = l + 1; i <= r; i++) {
        Song key = v[i];
        int j = i - 1;
        while (j >= l && goesBefore(key, v[j], field, asc)) { v[j + 1] = v[j]; j--; }
        v[j + 1] = key;
    }
}

void mergeSort(vector<Song> &v, int l, int r, int field, bool asc) {
    if (r - l < 8) { insertionSort(v, l, r, field, asc); return; }
    int m = (l + r) / 2;
    mergeSort(v, l, m, field, asc);
    mergeSort(v, m + 1, r, field, asc);
    vector<Song> tmp;
    int i = l, j = m + 1;
    while (i <= m && j <= r) {  // stable: take left unless right strictly goes first
        if (goesBefore(v[j], v[i], field, asc)) tmp.push_back(v[j++]);
        else tmp.push_back(v[i++]);
    }
    while (i <= m) tmp.push_back(v[i++]);
    while (j <= r) tmp.push_back(v[j++]);
    for (size_t k = 0; k < tmp.size(); k++) v[l + (int)k] = tmp[k];
}

void fisherYates(vector<Song> &v) {
    mt19937 rng((unsigned)chrono::steady_clock::now().time_since_epoch().count());
    for (int i = (int)v.size() - 1; i > 0; i--) {
        uniform_int_distribution<int> d(0, i);
        swap(v[i], v[d(rng)]);
    }
}

/* ======================================================================
 *  APPLICATION STATE
 * ====================================================================== */
struct Action {          // used by the undo stack
    int type;            // 0 = song was added, 1 = song was deleted
    string playlist;
    Song song;
    Action() : type(0) {}
};

class App {
public:
    vector<unique_ptr<Playlist> > lists;  // lists[0] is always the "Library"
    Queue<Song> playQueue;
    Stack<Song> history;
    Stack<Action> undoStack;
    Song nowPlaying;
    bool hasNow;
    int activeList;  // playlist used by the player

    App() : hasNow(false), activeList(0) {
        lists.push_back(unique_ptr<Playlist>(new Playlist("Library")));
    }

    Playlist &library() { return *lists[0]; }

    Playlist *findPlaylist(const string &name) {
        for (size_t i = 0; i < lists.size(); i++)
            if (toLower(lists[i]->name) == toLower(name)) return lists[i].get();
        return nullptr;
    }

    Playlist *choosePlaylist(const string &prompt) {
        for (size_t i = 0; i < lists.size(); i++)
            cout << "  " << i + 1 << ". " << lists[i]->name << " (" << lists[i]->count << " songs)\n";
        int c = askInt(prompt, 1, (int)lists.size());
        return lists[c - 1].get();
    }

    void buildIndexes(HashTable &byTitle, HashTable &byArtist, BST &tree) {
        vector<Song> v = library().toVector();
        for (size_t i = 0; i < v.size(); i++) {
            byTitle.insert(v[i].title, v[i]);
            byArtist.insert(v[i].artist, v[i]);
            tree.insert(v[i]);
        }
    }

    // ---------------- Library ----------------
    void addSong() {
        string t = ask("  Title : ");
        if (t.empty()) { cout << "  Title cannot be empty.\n"; return; }
        string a = ask("  Artist: ");
        string al = ask("  Album : ");
        string g = ask("  Genre : ");
        int d = askDuration();
        Song s(t, a, al, g, d);
        if (library().contains(s)) { cout << "  Duplicate song detected. Not added.\n"; return; }
        library().addBack(s);
        Action ac; ac.type = 0; ac.playlist = "Library"; ac.song = s;
        undoStack.push(ac);
        cout << "  Added \"" << s.title << "\".\n";
    }

    void deleteSong() {
        string t = ask("  Title of song to delete: ");
        Song removed;
        if (!library().removeByTitle(t, &removed)) { cout << "  Song not found.\n"; return; }
        for (size_t i = 1; i < lists.size(); i++) {  // also remove from playlists
            DNode *n;
            while ((n = lists[i]->findNode(removed.title)) != nullptr) lists[i]->removeNode(n);
        }
        Action ac; ac.type = 1; ac.playlist = "Library"; ac.song = removed;
        undoStack.push(ac);
        cout << "  Deleted \"" << removed.title << "\".\n";
    }

    void editSong() {
        string t = ask("  Title of song to edit: ");
        DNode *n = library().findNode(t);
        if (!n) { cout << "  Song not found.\n"; return; }
        Song old = n->data, upd = old;
        cout << "  (press Enter to keep the current value)\n";
        string s;
        s = ask("  Title [" + old.title + "]: ");  if (!s.empty()) upd.title = clean(s);
        s = ask("  Artist [" + old.artist + "]: "); if (!s.empty()) upd.artist = clean(s);
        s = ask("  Album [" + old.album + "]: ");   if (!s.empty()) upd.album = clean(s);
        s = ask("  Genre [" + old.genre + "]: ");   if (!s.empty()) upd.genre = clean(s);
        s = ask("  Duration [" + fmtDuration(old.duration) + "] (Enter to keep): ");
        if (!s.empty()) upd.duration = askDuration();
        for (size_t i = 0; i < lists.size(); i++) lists[i]->updateMatching(old, upd);
        cout << "  Song updated.\n";
    }

    void searchSong() {
        HashTable byTitle, byArtist;
        BST tree;
        buildIndexes(byTitle, byArtist, tree);
        int c = askInt("  Search by 1) Title  2) Artist : ", 1, 2);
        string key = ask("  Enter text: ");
        vector<Song> r = (c == 1) ? byTitle.search(key) : byArtist.search(key);
        if (r.empty()) { cout << "  No exact match found.\n"; return; }
        for (size_t i = 0; i < r.size(); i++)
            cout << "  * " << r[i].title << " - " << r[i].artist << " [" << r[i].album << ", "
                 << r[i].genre << ", " << fmtDuration(r[i].duration) << "]\n";
    }

    void showAlphabetical() {
        HashTable a, b;
        BST tree;
        buildIndexes(a, b, tree);
        vector<Song> v = tree.sorted();
        if (v.empty()) { cout << "  (library is empty)\n"; return; }
        for (size_t i = 0; i < v.size(); i++)
            cout << "  " << i + 1 << ". " << v[i].title << " - " << v[i].artist << "\n";
    }

    // ---------------- Playlists ----------------
    void createPlaylist() {
        string n = clean(ask("  New playlist name: "));
        if (n.empty() || findPlaylist(n)) { cout << "  Invalid or already existing name.\n"; return; }
        lists.push_back(unique_ptr<Playlist>(new Playlist(n)));
        cout << "  Playlist created.\n";
    }

    void renamePlaylist() {
        if (lists.size() < 2) { cout << "  No user playlists yet.\n"; return; }
        Playlist *p = choosePlaylist("  Playlist to rename: ");
        if (p == lists[0].get()) { cout << "  The Library cannot be renamed.\n"; return; }
        string n = clean(ask("  New name: "));
        if (n.empty() || findPlaylist(n)) { cout << "  Invalid or already existing name.\n"; return; }
        p->name = n;
        cout << "  Renamed.\n";
    }

    void deletePlaylist() {
        if (lists.size() < 2) { cout << "  No user playlists yet.\n"; return; }
        Playlist *p = choosePlaylist("  Playlist to delete: ");
        if (p == lists[0].get()) { cout << "  The Library cannot be deleted.\n"; return; }
        for (size_t i = 1; i < lists.size(); i++)
            if (lists[i].get() == p) {
                if ((int)i == activeList) activeList = 0;
                else if ((int)i < activeList) activeList--;
                lists.erase(lists.begin() + i);
                break;
            }
        cout << "  Playlist deleted.\n";
    }

    void addToPlaylist() {
        if (lists.size() < 2) { cout << "  Create a playlist first.\n"; return; }
        Playlist *p = choosePlaylist("  Add to which playlist: ");
        if (p == lists[0].get()) { cout << "  Use 'Add song' in the library menu.\n"; return; }
        string t = ask("  Title of library song: ");
        DNode *n = library().findNode(t);
        if (!n) { cout << "  Song not found in library.\n"; return; }
        if (p->contains(n->data)) { cout << "  Duplicate song detected. Not added.\n"; return; }
        p->addBack(n->data);
        Action ac; ac.type = 0; ac.playlist = p->name; ac.song = n->data;
        undoStack.push(ac);
        cout << "  Added to \"" << p->name << "\".\n";
    }

    void removeFromPlaylist() {
        if (lists.size() < 2) { cout << "  No user playlists yet.\n"; return; }
        Playlist *p = choosePlaylist("  Remove from which playlist: ");
        if (p == lists[0].get()) { cout << "  Use 'Delete song' in the library menu.\n"; return; }
        string t = ask("  Title of song: ");
        Song removed;
        if (!p->removeByTitle(t, &removed)) { cout << "  Song not in this playlist.\n"; return; }
        Action ac; ac.type = 1; ac.playlist = p->name; ac.song = removed;
        undoStack.push(ac);
        cout << "  Removed.\n";
    }

    void undo() {
        Action ac;
        if (!undoStack.pop(ac)) { cout << "  Nothing to undo.\n"; return; }
        Playlist *p = findPlaylist(ac.playlist);
        if (!p) { cout << "  The playlist no longer exists.\n"; return; }
        if (ac.type == 0) { p->removeByTitle(ac.song.title); cout << "  Undone: addition of \"" << ac.song.title << "\".\n"; }
        else { p->addBack(ac.song); cout << "  Undone: deletion of \"" << ac.song.title << "\".\n"; }
    }

    // ---------------- Player ----------------
    void play(const Song &s) {
        nowPlaying = s;
        hasNow = true;
        history.push(s);
        cout << "  >> Now playing: " << s.title << " - " << s.artist << " (" << fmtDuration(s.duration) << ")\n";
    }

    void playNext() {
        Song *s = lists[activeList]->next();
        if (!s) { cout << "  The active playlist is empty.\n"; return; }
        play(*s);
    }

    void playPrevious() {
        Song *s = lists[activeList]->prev();
        if (!s) { cout << "  The active playlist is empty.\n"; return; }
        play(*s);
    }

    void playFromQueue() {
        Song s;
        if (!playQueue.dequeue(s)) { cout << "  The play queue is empty.\n"; return; }
        play(s);
    }

    void enqueueSong() {
        string t = ask("  Title (from library) to enqueue: ");
        DNode *n = library().findNode(t);
        if (!n) { cout << "  Song not found.\n"; return; }
        playQueue.enqueue(n->data);
        cout << "  Enqueued.\n";
    }

    void dequeueSong() {
        Song s;
        if (!playQueue.dequeue(s)) { cout << "  The play queue is empty.\n"; return; }
        cout << "  Removed from queue: " << s.title << "\n";
    }

    void showQueue() {
        vector<Song> v = playQueue.toVector();
        if (v.empty()) { cout << "  (queue is empty)\n"; return; }
        for (size_t i = 0; i < v.size(); i++) cout << "  " << i + 1 << ". " << v[i].title << " - " << v[i].artist << "\n";
    }

    void showHistory() {
        vector<Song> v = history.toVector();
        if (v.empty()) { cout << "  (no songs played yet)\n"; return; }
        for (size_t i = 0; i < v.size(); i++) cout << "  " << i + 1 << ". " << v[i].title << " - " << v[i].artist << "\n";
    }

    void playLastPlayed() {  // go back to the previously played song using the stack
        if (history.size() < 2) { cout << "  No earlier song in history.\n"; return; }
        Song cur, prev;
        history.pop(cur);
        history.peek(prev);
        nowPlaying = prev;
        hasNow = true;
        cout << "  >> Back to: " << prev.title << " - " << prev.artist << "\n";
    }

    void shufflePlay() {
        vector<Song> v = lists[activeList]->toVector();
        if (v.empty()) { cout << "  The active playlist is empty.\n"; return; }
        fisherYates(v);
        playQueue.clear();
        for (size_t i = 0; i < v.size(); i++) playQueue.enqueue(v[i]);
        cout << "  Shuffled " << v.size() << " songs into the play queue.\n";
        playFromQueue();
    }

    void chooseActive() {
        Playlist *p = choosePlaylist("  Select active playlist: ");
        for (size_t i = 0; i < lists.size(); i++) if (lists[i].get() == p) activeList = (int)i;
        cout << "  Active playlist: " << p->name << "\n";
    }

    // ---------------- Sorting ----------------
    void sortPlaylist() {
        Playlist *p = choosePlaylist("  Sort which playlist: ");
        if (p->count < 2) { cout << "  Nothing to sort.\n"; return; }
        int f = askInt("  Sort by 1) Title  2) Artist  3) Duration : ", 1, 3);
        int o = askInt("  Order 1) Ascending  2) Descending : ", 1, 2);
        vector<Song> v = p->toVector();
        mergeSort(v, 0, (int)v.size() - 1, f, o == 1);
        p->rebuild(v);
        cout << "  Sorted.\n";
        p->display();
    }

    // ---------------- File handling ----------------
    bool save(const string &file = "playlists.txt") {
        ofstream out(file.c_str());
        if (!out) return false;
        for (size_t i = 0; i < lists.size(); i++) {
            vector<Song> v = lists[i]->toVector();
            out << "PLAYLIST|" << lists[i]->name << "|" << v.size() << "\n";
            for (size_t j = 0; j < v.size(); j++)
                out << v[j].title << "|" << v[j].artist << "|" << v[j].album << "|" << v[j].genre << "|" << v[j].duration << "\n";
        }
        return true;
    }

    static vector<string> split(const string &line) {
        vector<string> parts;
        string cur;
        for (size_t i = 0; i < line.size(); i++) {
            if (line[i] == '|') { parts.push_back(cur); cur.clear(); }
            else cur += line[i];
        }
        parts.push_back(cur);
        return parts;
    }

    bool load(const string &file = "playlists.txt") {
        ifstream in(file.c_str());
        if (!in) return false;
        lists.clear();
        playQueue.clear();
        history.clear();
        undoStack.clear();
        hasNow = false;
        activeList = 0;
        string line;
        Playlist *curList = nullptr;
        int remaining = 0;
        while (getline(in, line)) {
            if (line.empty()) continue;
            vector<string> p = split(line);
            if (p[0] == "PLAYLIST" && p.size() >= 3 && remaining == 0) {
                lists.push_back(unique_ptr<Playlist>(new Playlist(p[1])));
                curList = lists.back().get();
                try { remaining = stoi(p[2]); } catch (...) { remaining = 0; }
            } else if (curList && p.size() >= 5 && remaining > 0) {
                int d = 0;
                try { d = stoi(p[4]); } catch (...) {}
                curList->addBack(Song(p[0], p[1], p[2], p[3], d));
                remaining--;
            }
        }
        if (lists.empty() || lists[0]->name != "Library")
            lists.insert(lists.begin(), unique_ptr<Playlist>(new Playlist("Library")));
        return true;
    }

    void loadSamples() {
        Song s[] = {
            Song("Tum Hi Ho", "Arijit Singh", "Aashiqui 2", "Romantic", 262),
            Song("Kesariya", "Arijit Singh", "Brahmastra", "Romantic", 268),
            Song("Excuses", "AP Dhillon", "The Brownprint", "Punjabi", 177),
            Song("Brown Munde", "AP Dhillon", "Brown Munde", "Punjabi", 267),
            Song("Lover", "Diljit Dosanjh", "Lover", "Punjabi", 191),
            Song("Chaiyya Chaiyya", "Sukhwinder Singh", "Dil Se", "Bollywood", 413),
            Song("Blinding Lights", "The Weeknd", "After Hours", "Pop", 200),
            Song("Shape of You", "Ed Sheeran", "Divide", "Pop", 234),
            Song("Believer", "Imagine Dragons", "Evolve", "Rock", 204),
            Song("Kun Faya Kun", "A. R. Rahman", "Rockstar", "Sufi", 428)
        };
        for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); i++)
            if (!library().contains(s[i])) library().addBack(s[i]);
        Playlist *fav = findPlaylist("Favourites");
        if (!fav) {
            lists.push_back(unique_ptr<Playlist>(new Playlist("Favourites")));
            fav = lists.back().get();
        }
        for (int i = 0; i < 4; i++) if (!fav->contains(s[i])) fav->addBack(s[i]);
    }

    // ---------------- HTML export (styled by style.css) ----------------
    void songTable(ofstream &o, const vector<Song> &v) {
        if (v.empty()) { o << "<p class=\"empty\">No songs.</p>\n"; return; }
        o << "<div class=\"table-wrap\"><table>\n<thead><tr><th>#</th><th>Title</th><th>Artist</th>"
             "<th>Album</th><th>Genre</th><th>Duration</th></tr></thead>\n<tbody>\n";
        for (size_t i = 0; i < v.size(); i++)
            o << "<tr><td>" << i + 1 << "</td><td>" << htmlEscape(v[i].title) << "</td><td>"
              << htmlEscape(v[i].artist) << "</td><td>" << htmlEscape(v[i].album) << "</td><td><span class=\"tag\">"
              << htmlEscape(v[i].genre) << "</span></td><td>" << fmtDuration(v[i].duration) << "</td></tr>\n";
        o << "</tbody></table></div>\n";
    }

    void miniList(ofstream &o, const vector<Song> &v) {
        if (v.empty()) { o << "<p class=\"empty\">Empty.</p>\n"; return; }
        o << "<ol class=\"mini\">\n";
        for (size_t i = 0; i < v.size(); i++)
            o << "<li><strong>" << htmlEscape(v[i].title) << "</strong><span>" << htmlEscape(v[i].artist) << "</span></li>\n";
        o << "</ol>\n";
    }

    bool exportHtml(const string &file = "index.html") {
        ofstream o(file.c_str());
        if (!o) return false;
        int total = 0;
        for (size_t i = 0; i < lists.size(); i++) total += lists[i]->count;

        o << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"UTF-8\">\n"
             "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
             "<title>Music Playlist Organizer</title>\n<link rel=\"stylesheet\" href=\"style.css\">\n</head>\n<body>\n";

        o << "<header class=\"hero\"><h1>&#9835; Music Playlist Organizer</h1>"
             "<p>A Data Structure Based C++ Application &middot; BCA 3rd Semester</p></header>\n<main>\n";

        o << "<section class=\"stats\">\n"
          << "<div class=\"stat\"><b>" << library().count << "</b><span>Songs in library</span></div>\n"
          << "<div class=\"stat\"><b>" << lists.size() - 1 << "</b><span>Playlists</span></div>\n"
          << "<div class=\"stat\"><b>" << playQueue.size() << "</b><span>In play queue</span></div>\n"
          << "<div class=\"stat\"><b>" << history.size() << "</b><span>Songs played</span></div>\n</section>\n";

        o << "<section class=\"now card\"><h2>Now Playing</h2>\n";
        if (hasNow)
            o << "<p class=\"now-title\">" << htmlEscape(nowPlaying.title) << "</p><p>" << htmlEscape(nowPlaying.artist)
              << " &middot; " << htmlEscape(nowPlaying.album) << " &middot; " << fmtDuration(nowPlaying.duration) << "</p>\n"
              << "<div class=\"bar\"><div class=\"fill\"></div></div>\n";
        else o << "<p class=\"empty\">Nothing is playing.</p>\n";
        o << "</section>\n";

        for (size_t i = 0; i < lists.size(); i++) {
            int secs = 0;
            vector<Song> v = lists[i]->toVector();
            for (size_t j = 0; j < v.size(); j++) secs += v[j].duration;
            o << "<section class=\"card\"><h2>" << htmlEscape(lists[i]->name) << " <small>" << v.size()
              << " songs &middot; " << fmtDuration(secs) << " total</small></h2>\n";
            songTable(o, v);
            o << "</section>\n";
        }

        HashTable a, b;
        BST tree;
        buildIndexes(a, b, tree);

        o << "<div class=\"grid\">\n";
        o << "<section class=\"card\"><h2>Up Next <small>queue (FIFO)</small></h2>\n";
        miniList(o, playQueue.toVector());
        o << "</section>\n<section class=\"card\"><h2>Recently Played <small>stack (LIFO)</small></h2>\n";
        miniList(o, history.toVector());
        o << "</section>\n<section class=\"card\"><h2>A &ndash; Z <small>binary search tree</small></h2>\n";
        miniList(o, tree.sorted());
        o << "</section>\n</div>\n";

        o << "</main>\n<footer>Generated by the Music Playlist Organizer (C++). Total entries: " << total
          << "</footer>\n</body>\n</html>\n";
        return true;
    }
};

/* ======================================================================
 *  MENUS
 * ====================================================================== */
void libraryMenu(App &app) {
    while (true) {
        cout << "\n--- SONG LIBRARY ---\n"
                "  1. Add song          2. Delete song      3. Edit song\n"
                "  4. Display library   5. Search (hash)    6. A-Z view (BST)\n"
                "  7. Undo last add/delete                  0. Back\n";
        int c = askInt("Choice: ", 0, 7);
        if (c == 0) return;
        switch (c) {
            case 1: app.addSong(); break;
            case 2: app.deleteSong(); break;
            case 3: app.editSong(); break;
            case 4: app.library().display(); break;
            case 5: app.searchSong(); break;
            case 6: app.showAlphabetical(); break;
            case 7: app.undo(); break;
        }
    }
}

void playlistMenu(App &app) {
    while (true) {
        cout << "\n--- PLAYLISTS ---\n"
                "  1. Create playlist   2. Rename playlist  3. Delete playlist\n"
                "  4. Add song to playlist                  5. Remove song from playlist\n"
                "  6. Display a playlist                    7. Sort a playlist\n"
                "  8. Undo last add/delete                  0. Back\n";
        int c = askInt("Choice: ", 0, 8);
        if (c == 0) return;
        switch (c) {
            case 1: app.createPlaylist(); break;
            case 2: app.renamePlaylist(); break;
            case 3: app.deletePlaylist(); break;
            case 4: app.addToPlaylist(); break;
            case 5: app.removeFromPlaylist(); break;
            case 6: { Playlist *p = app.choosePlaylist("  Show which playlist: "); cout << "\n  [" << p->name << "]\n"; p->display(); break; }
            case 7: app.sortPlaylist(); break;
            case 8: app.undo(); break;
        }
    }
}

void playerMenu(App &app) {
    while (true) {
        cout << "\n--- PLAYER (active playlist: " << app.lists[app.activeList]->name << ") ---\n"
                "  1. Choose active playlist   2. Play next (repeat-all)  3. Play previous\n"
                "  4. Shuffle play             5. Enqueue song            6. Dequeue song\n"
                "  7. Play from queue          8. Show queue              9. Recently played\n"
                " 10. Go back to last played   0. Back\n";
        int c = askInt("Choice: ", 0, 10);
        if (c == 0) return;
        switch (c) {
            case 1: app.chooseActive(); break;
            case 2: app.playNext(); break;
            case 3: app.playPrevious(); break;
            case 4: app.shufflePlay(); break;
            case 5: app.enqueueSong(); break;
            case 6: app.dequeueSong(); break;
            case 7: app.playFromQueue(); break;
            case 8: app.showQueue(); break;
            case 9: app.showHistory(); break;
            case 10: app.playLastPlayed(); break;
        }
    }
}

int main() {
    App app;
    if (app.load()) cout << "Loaded saved data from playlists.txt\n";
    else { app.loadSamples(); cout << "No saved data found. Sample songs loaded.\n"; }

    while (true) {
        cout << "\n==========================================\n"
                "        MUSIC PLAYLIST ORGANIZER\n"
                "==========================================\n"
                "  1. Song library\n"
                "  2. Playlists\n"
                "  3. Player & queue\n"
                "  4. Save data to file\n"
                "  5. Load data from file\n"
                "  6. Export HTML dashboard (index.html)\n"
                "  7. Load sample songs\n"
                "  0. Save & exit\n";
        int c = askInt("Choice: ", 0, 7);
        switch (c) {
            case 1: libraryMenu(app); break;
            case 2: playlistMenu(app); break;
            case 3: playerMenu(app); break;
            case 4: cout << (app.save() ? "  Saved to playlists.txt\n" : "  Could not save file.\n"); break;
            case 5: cout << (app.load() ? "  Loaded.\n" : "  No saved file found.\n"); break;
            case 6: cout << (app.exportHtml() ? "  index.html created. Open it in a browser.\n" : "  Could not write index.html\n"); break;
            case 7: app.loadSamples(); cout << "  Sample songs added.\n"; break;
            case 0:
                app.save();
                app.exportHtml();
                cout << "Data saved. Goodbye!\n";
                return 0;
        }
    }
}
