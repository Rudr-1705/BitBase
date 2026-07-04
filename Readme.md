
# BitBase CLI

BitBase is a small, from-scratch SQL database engine and REPL, written in C++17.
It implements its own tokenizer, parser, executor, on-disk storage format, B+
tree index, pager, and write-ahead log — no external database library is used
anywhere in the project.

This document covers:

1. [Quick start](#1-quick-start)
2. [High-level architecture](#2-high-level-architecture)
3. [On-disk layout](#3-on-disk-layout)
4. [Module-by-module reference](#4-module-by-module-reference)
5. [SQL grammar supported](#5-sql-grammar-supported)
6. [Known design constraints](#6-known-design-constraints)
7. [Running the test suite](#7-running-the-test-suite)
8. [Troubleshooting](#8-troubleshooting)

---

## 1. Quick start

### Prerequisites

- A C++17 compiler (g++/clang++ on Linux/macOS, or MinGW-w64/MSVC on Windows)
- CMake >= 3.16

```bash
# Ubuntu/Debian
sudo apt update && sudo apt install build-essential cmake

# macOS
brew install cmake
```

### Build

```bash
cd bitbase_cli
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

This produces `bitbase_cli` (or `bitbase_cli.exe` on Windows) inside `build/`
(the exact sub-path depends on your CMake generator — check with
`find . -name "bitbase_cli*"` if unsure).

### Run

```bash
./bitbase_cli
```

```sql
Bitbase> create table users (id INT primary key, name TEXT, email TEXT unique);
Bitbase> insert into users values (1, 'Alice', 'alice@example.com');
Bitbase> select * from users;
(1, Alice, alice@example.com)
Bitbase> .exit
```

**Important:** SQL keywords (`select`, `insert`, `create`, `where`, `and`,
`order`, `by`, `limit`, `set`, `values`, `into`, `from`, `table`, `primary`,
`key`, `unique`, `drop`) must be typed **lowercase**. Data type names (`INT`,
`BIGINT`, `FLOAT`, `DOUBLE`, `BOOL`, `TEXT`) must be typed **uppercase**. See
[§6](#6-known-design-constraints) for why.

A `data/` folder and a `wal.log` file are created in whatever directory you
launch the executable from — this is where all table files and the catalog
live. Run the binary from a consistent working directory if you want your
data to persist between sessions.

---

## 2. High-level architecture

```
┌─────────────────────────────────────────────────────────────┐
│                            main()                             │
└──────────────────────────────┬────────────────────────────────┘
                                 │
                         ┌───────▼────────┐
                         │      Repl       │  read–eval–print loop
                         │  (repl.cpp)     │
                         └───┬─────────┬───┘
                             │         │
              starts with '.'│         │everything else
                             │         │
                  ┌──────────▼──┐   ┌──▼────────────┐
                  │ MetaCommand  │   │    Parser      │  tokenize + parse
                  │  Handler     │   │  (parser.cpp)  │  → Statement
                  │ (meta.cpp)   │   └───────┬────────┘
                  └──────────────┘           │
                                     ┌────────▼─────────┐
                                     │     Executor       │  dispatches on
                                     │  (executor.cpp)     │  Statement::type
                                     └───┬─────────────┬──┘
                                         │             │
                              ┌──────────▼──┐   ┌──────▼───────┐
                              │  WALManager  │   │   Database    │
                              │  (wal.cpp)   │   │ (database.cpp)│
                              └──────────────┘   └──────┬────────┘
                                                          │  owns
                                                  ┌───────▼────────┐
                                                  │     Table       │  one per
                                                  │  (table.cpp)    │  SQL table
                                                  └───┬─────────┬──┘
                                                      │         │
                                          ┌───────────▼──┐  ┌───▼──────────┐
                                          │    Pager      │  │   B+ Tree     │
                                          │ (pager.cpp)   │  │  (node.cpp)   │
                                          └───────────────┘  └───────────────┘
```

**Request flow for a typical command** (e.g. `select * from users where id = 1;`):

1. `Repl::start()` reads a line from stdin.
2. If it starts with `.`, `MetaCommandHandler::handle()` processes it (`.exit`, `.tables`, `.help`).
3. Otherwise, `Parser::parse()` tokenizes the line and fills out a `Statement` struct describing what to do.
4. `Executor::execute()` switches on `statement.type` and calls into the right `Table` methods on the right table (looked up via `Database::get_table()`).
5. `Table` methods read/write raw bytes through the `Pager`, and — for primary-key tables — maintain a B+ tree index (`node.cpp`) for fast point/range lookups.
6. Mutating statements (INSERT/UPDATE/DELETE) also get logged to `wal.log` via `WALManager`, after the storage-level operation succeeds.

---

## 3. On-disk layout

When you run `bitbase_cli` from directory `X`, it creates:

```
X/
├── data/
│   ├── bitbase.meta      ← catalog: plain-text list of table names
│   ├── users.db          ← one binary file per table
│   ├── orders.db
│   └── ...
└── wal.log               ← write-ahead log (append-only text log)
```

### Table file (`<table>.db`) layout

Each table file is a flat sequence of fixed-size 4096-byte **pages**,
addressed by page number starting at 0:

| Page        | Purpose                                                                                                                                                                                                                                        |
| ----------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Page 0**  | Metadata page: `[num_rows: u32][schema_size: u32][serialized schema bytes...]`                                                                                                                                                                 |
| **Page 1**  | Root of the B+ tree index (only meaningful if the table has a `PRIMARY KEY`)                                                                                                                                                                   |
| **Page 2+** | Row heap — each page starts with a 4-byte "bytes used" counter, followed by a sequence of `[row_size: i32][row bytes...]` records. A negative `row_size` marks that record as **tombstoned** (deleted) without physically compacting the page. |

Tables **without** a primary key still get a valid (empty) B+ tree root page —
it's just never populated, since there's no key to index on. All reads for
such tables go through a full sequential scan of the row heap
(`Table::get_all_dynamic()`).

### Row encoding

Rows are encoded dynamically based on the table's `Schema` (see
`dynamic_row_format.cpp`) — there's no fixed-width row struct. Each column is
serialized according to its `DataType`:

| DataType | Encoding                 |
| -------- | ------------------------ |
| `INT32`  | 4-byte little-endian int |
| `INT64`  | 8-byte little-endian int |
| `FLOAT`  | 4-byte IEEE-754          |
| `DOUBLE` | 8-byte IEEE-754          |
| `BOOL`   | 1 byte                   |
| `TEXT`   | length-prefixed string   |

### Schema encoding (on-disk)

```
[column_count: u32]
for each column:
    [name_len: u32][name bytes][type: u8][flags: u8]
```

`flags` bit 0 = `is_primary`, bit 1 = `is_unique`. (Prior to the July 2026
bugfix pass, these flags were **not** persisted at all — see the CHANGELOG
note in [§6](#6-known-design-constraints) if you're working with `.db` files
created by an older build; they are not forward-compatible with this format.)

---

## 4. Module-by-module reference

### `main.cpp`
Entry point. Constructs a `Repl` and calls `start()`.

### `repl/` — Read-Eval-Print Loop

**`Repl::start()`**
Owns the long-lived `MetaCommandHandler`, `Parser`, and `Executor` instances
for the whole session. Loop body:
- print `Bitbase> `, read a line
- empty line → skip
- line starts with `.` → hand to `MetaCommandHandler`; `.exit` breaks the loop
- otherwise → `Parser::parse()`, then `Executor::execute()` on success, or
  print the parser's error message on failure

### `meta/` — Dot-commands

**`MetaCommandHandler::handle(input, db)`** → `MetaCommandResult`
| Command       | Behavior                                                                    |
| ------------- | --------------------------------------------------------------------------- |
| `.exit`       | returns `EXIT`, which breaks the REPL loop                                  |
| `.tables`     | prints every table name currently in `db.tables`                            |
| `.help`       | prints a short static command list                                          |
| anything else | returns `UNRECOGNIZED` (silently ignored by the REPL — no error is printed) |

### `parser/` — Tokenizer + recursive-descent-ish parser

**`tokenize(input)`** (in `utils/tokenizer.cpp`)
Splits a line into tokens. Rules:
- whitespace separates tokens and is discarded
- `(`, `)`, `,`, `;` are always their own single-character tokens, regardless
  of surrounding whitespace (so `(id,name)` and `( id , name )` tokenize
  identically)
- text inside `'single quotes'` is captured verbatim as one token (quotes
  stripped later by the parser), so string literals may contain spaces
- everything else accumulates into a token until whitespace or one of the
  above punctuation marks is hit — **this means multi-character operators
  like `>=`/`<=` only tokenize correctly if separated from the number by
  whitespace on at least one side**, and unquoted identifiers/keywords are
  compared byte-for-byte (see [§6](#6-known-design-constraints))

**`Parser::parse(input, statement, error)`** → `bool`
Looks at `tokens[0]` and dispatches to one of six hand-written statement
parsers (INSERT / SELECT / DELETE / UPDATE / CREATE TABLE / DROP TABLE),
filling in a `Statement` struct (see `parser/statement.h`). Returns `false`
with a human-readable `error` string for malformed input. Trailing `;` tokens
are simply left unconsumed and ignored — semicolons are optional.

Notable per-statement quirks:
- **SELECT**'s `WHERE` clause has a special fast path: if the condition is
  exactly `id >= N and id <= M`, it's parsed as a range query
  (`statement.is_range`) instead of a generic condition list, so it can use
  the B+ tree's range scan instead of a full table scan. Any other `WHERE`
  shape (including a plain `id = N`) falls back to generic `AND`-chained
  conditions in `statement.conditions`.
- Generic `WHERE`/condition parsing supports `=`, `!=`, `>`, `<`, `>=`, `<=`
  and any number of `AND`-joined clauses (no `OR`, no parentheses/grouping).
- **CREATE TABLE** allows at most one `PRIMARY KEY` column (a second one is a
  parse error) and any number of `UNIQUE` columns. A column can be
  `PRIMARY KEY` or `UNIQUE`, not both, in the current grammar.
- **UPDATE** syntax is fixed-position: `update <table> set <col> = <val> [where ...]`
  — only a single column can be set per statement.

### `executor/` — Statement dispatcher

**`Executor::Executor()`**
Constructs the `Database` (which loads the on-disk catalog) and opens
`wal.log` for append.

**`Executor::execute(statement)`**
One `case` per `StatementType`:
- **INSERT** — looks up the table, checks column count matches the schema,
  runs a `UNIQUE`-constraint pre-check across *all* unique columns (via
  `Table::exists_value_in_column`), then calls `Table::insert()`. On success,
  appends an entry to the WAL.
- **SELECT** — chooses a data-access strategy in this priority order:
  range query (B+ tree) → point lookup by primary key (`find_all_by_id`, if
  the `WHERE` clause pins the PK column with `=`) → full scan
  (`get_all_dynamic`). Generic conditions are then applied via
  `Table::filter_rows`. `ORDER BY` and `LIMIT` are applied last, in that
  order, after filtering.
- **DELETE** — no `WHERE` → `Table::delete_all()`. With `WHERE` →
  `Table::delete_where_full()`, which does a full-scan filter and tombstones
  matching rows (removing them from the B+ tree too, if the table has a PK).
- **UPDATE** — always goes through `Table::update_where()` (a full-scan +
  filter + in-place-or-reinsert rewrite), which enforces both "can't update
  the primary key column" and "can't violate a UNIQUE constraint."
- **CREATE_TABLE** — `Database::create_table()` then `Table::set_schema()`
  (which persists the schema to the table's metadata page immediately).
- **DROP_TABLE** — `Database::drop_table()` deletes the in-memory `Table`,
  removes the `.db` file from disk, and rewrites the catalog.

### `storage/database/` — Catalog + table registry

**`Database`** owns an `unordered_map<string, Table*>` and a plain-text
catalog file (`data/bitbase.meta`, one table name per line).

- `Database()` — creates `data/` if missing, then `load_catalog()`
- `load_catalog()` — reads the catalog file and opens (constructs) a `Table`
  for every listed name; **does not** call `rebuild_index()` on reload,
  because the B+ tree pages are already persisted as part of each `.db` file
  — there's nothing to rebuild
- `create_table(name)` — refuses if the name is already taken; otherwise
  creates a fresh `Table`, runs (a no-op, since it's empty) `rebuild_index()`,
  and rewrites the catalog
- `drop_table(name)` — destructs the `Table` object (flushing/closing its
  file), deletes the `.db` file, rewrites the catalog
- `get_table(name)` — map lookup, returns `nullptr` if not found

### `storage/table/` — Per-table storage engine

This is the biggest and most important module. A `Table` owns one `Pager`
(one open file) and an in-memory `Schema`.

**Construction (`Table::Table(filename)`)**
- Opens (or creates) the file via `Pager`
- If the file is brand new (`file_length == 0`): zeroes page 0's `num_rows`
  and `schema_size` fields
- Otherwise: reads `num_rows` and deserializes the `Schema` straight out of
  page 0
- Ensures page 1 is initialized as a valid B+ tree leaf node if it isn't
  already (first-time setup)

**`insert(values)`** → `bool`
1. If the table has a primary key, parses the PK value as `u32` and does a
   full-scan duplicate check against `get_all_dynamic()` (not the B+ tree —
   this is the "real source of truth" per an in-code comment, precisely to
   avoid depending on index state)
2. Serializes the row via `serialize_dynamic_row()` and appends it to the
   first row-heap page with enough free space (never compacts/reuses
   tombstoned space — the heap only grows)
3. If there's a primary key, also inserts `{key → RowPointer}` into the B+
   tree (`btree_insert`), splitting/creating a new root as needed
4. Bumps and persists `num_rows`

**`get_all_dynamic()`** → full sequential scan of every row-heap page,
skipping tombstoned (negative-size) records. This is the fallback path used
whenever there's no usable index, and is also used internally by the
duplicate-key check above.

**`scan_all_index()`** → walks the B+ tree via `find_leftmost_leaf` +
`leaf_node_next_leaf` chaining to visit every row **in primary-key order**.
Returns nothing useful on a table without a primary key (empty tree).

**`exists_value_in_column(col_idx, value)`** → `bool`
Used for `UNIQUE` enforcement. Does a full scan (`get_all_dynamic()`) and
string-compares the target column. Deliberately does **not** use
`scan_all_index()`, because that would silently return nothing on tables
without a primary key, breaking `UNIQUE` on such tables (this was a real bug,
fixed — see [§6](#6-known-design-constraints)).

**`find_by_id` / `find_all_by_id` / `range_query`** → B+ tree point/range
lookups. Require a primary key to return anything meaningful.

**`delete_by_id(key)`** → finds the row pointer via the B+ tree, flips the
row's size field negative (tombstone) in the heap, then removes the key from
the B+ tree.

**`delete_all()`** / **`delete_where_full(conds)`** → full-scan tombstoning;
the `WHERE` variant re-derives each matching row's PK (if any) and calls
`delete_by_id` to also clean up the index, or tombstones directly for
PK-less tables.

**`update_by_id(key, column, value)`** — B+ tree-based single-row update path.
**Not** the path the executor actually uses for `UPDATE` statements (see
`update_where` below) but kept as a lower-level primitive; enforces PK and
UNIQUE checks identically to `update_where`.

**`update_where(conds, column, value)`** — the path the executor calls for
every `UPDATE`. Full scan + `filter_rows` per candidate row; refuses to touch
the primary-key column at all; enforces `UNIQUE` before applying the change.
Rewrites in place if the new serialized row is the same size or smaller;
otherwise tombstones the old row and calls `insert()` for the new one
(meaning the row's physical location, and therefore B+ tree pointer, changes
on size growth — this is why the check happens before serialization).

**`filter_rows(rows, conds)`** → applies an `AND`-chain of `Statement::Condition`
against already-materialized rows, dispatching comparisons per column
`DataType` (numeric columns compare numerically; `TEXT`/`BOOL` compare as
strings). Supports `=`, `!=`, `>`, `<`, `>=`, `<=`.

**`order_rows(rows, column)`** → in-memory sort by one column (ascending only;
no `DESC` keyword in the grammar).

**`rebuild_index()`** → resets the B+ tree root and, if the table has a
primary key, walks every row via `get_all_dynamic()` and re-inserts each into
a fresh tree. Called automatically whenever a table is created.

### `storage/schema/` — Column & schema metadata

- **`value.h`** — `DataType` enum (`INT32, INT64, FLOAT, DOUBLE, BOOL, TEXT`)
  and `Value = std::variant<int32_t, int64_t, float, double, bool, std::string>`
- **`column.h`** — `Column { name, type, is_primary, is_unique }`
- **`schema.cpp`** — `Schema` wraps `vector<Column>` plus:
  - `add_column`, `get_column_index`, `get_primary_index` (linear scans)
  - `serialize()` / `deserialize()` — binary (de)serialization used to persist
    the schema in each table's metadata page (format described in
    [§3](#3-on-disk-layout))

### `storage/row_format/`

**`dynamic_row_format.cpp`** — `serialize_dynamic_row` / `deserialize_dynamic_row`:
the schema-driven row (de)serializer used by every `Table` operation. Each
column is packed/unpacked according to its `DataType` (see the encoding table
in [§3](#3-on-disk-layout)).

### `storage/pager/` — Page cache / file I/O

**`Pager`** wraps a single `FILE*` and an in-memory array of loaded 4KB pages
(`pages[TABLE_MAX_PAGES]`, up to 10,000 pages ≈ 40MB per table file).
- `get_page(n)` — lazily loads page `n` from disk into `pages[n]` on first
  access (zero-filling if it's a brand-new page past current EOF); returns
  the cached pointer on subsequent calls
- `flush(n)` — writes `pages[n]` back to disk at the right offset
- `allocate_page()` — hands out the next page number for **row-heap** data
  (grows monotonically from wherever row storage starts)
- `allocate_btree_page()` — hands out the next page number for **B+ tree**
  nodes (tracked separately via `next_btree_page`, starting at page 2)

Pages are never evicted from memory once loaded — the whole table's touched
pages stay resident for the process lifetime.

### `storage/btree/` — B+ tree index (`node.cpp` / `node.h`)

A textbook disk-based B+ tree, one tree per table, keyed on the primary key
(as a `u32`):
- **Leaf nodes**: header `[num_cells][next_leaf]` + cells of
  `[key][page_id][offset]` (a `RowPointer` into the row heap)
- **Internal nodes**: header `[num_keys][right_child]` + cells of
  `[child_page][key]`
- `btree_insert` — recursive insert with node splitting on overflow;
  `create_new_root` builds a new internal root when the existing root splits
- `btree_find` / `btree_find_leaf` / `leaf_search` — point lookup
- `btree_delete` / `leaf_delete` — key removal (leaf-level only; this
  implementation does not rebalance/merge underfull nodes after deletion,
  it just removes the cell)
- `find_leftmost_leaf` + `leaf_node_next_leaf` — leaf-chain traversal used for
  full-index scans and range queries, since leaves are linked left-to-right

### `storage/wal/` — Write-ahead log

**`WALManager`** appends human-readable log lines to `wal.log` for every
successful INSERT/DELETE/UPDATE (`log_insert`, `log_delete`, `log_update`),
flushed immediately after each write. This gives you a plain-text audit trail
of every mutating statement executed against the database. Table data
durability itself comes from the row-heap being written directly to each
table's own `.db` file on every `insert()`/`flush()` call.

### `utils/`

- `tokenize()` — see [§4 parser](#parser--tokenizer--recursive-descent-ish-parser) above

---

## 5. SQL grammar supported

```sql
CREATE TABLE <name> (
    <col> <TYPE> [PRIMARY KEY | UNIQUE],
    <col> <TYPE> [PRIMARY KEY | UNIQUE],
    ...
);

DROP TABLE <name>;

INSERT INTO <name> VALUES (<val>, <val>, ...);

SELECT * | <col>, <col>, ... FROM <name>
    [WHERE <col> <op> <val> [AND <col> <op> <val> ...]]
    [ORDER BY <col>]
    [LIMIT <n>];

UPDATE <name> SET <col> = <val> [WHERE <col> <op> <val> [AND ...]];

DELETE FROM <name> [WHERE <col> <op> <val> [AND ...]];
```

- `<TYPE>` ∈ `{ INT, BIGINT, FLOAT, DOUBLE, BOOL, TEXT }` (uppercase)
- `<op>` ∈ `{ =, !=, >, <, >=, <= }`
- String literals use `'single quotes'` and may contain spaces
- `;` is optional
- No `OR`, no parentheses/grouping in `WHERE`, no `JOIN`, no aggregate
  functions, no `GROUP BY`, no `DESC` sort order, no `NULL` literal in SQL
  text (columns can hold no `NULL` — every column must get a value on
  `INSERT`)
- Special case: `WHERE id >= N AND id <= M` is recognized as a contiguous
  range and served from the B+ tree; every other `WHERE` shape does a full
  table scan (with a single-row B+ tree point lookup as an optimization for
  a bare `WHERE id = N`)

Meta commands (must start with `.`, no trailing `;`):
```
.exit       exit the REPL
.tables     list all table names
.help       print available commands
```

---

## 6. Known design constraints

These are things you'll likely bump into — noted here so they read as
"documented behavior" rather than mysterious bugs:

- **Keyword case-sensitivity is split by design.** SQL keywords (`select`,
  `create`, `where`, `and`, etc.) must be lowercase; type names (`INT`,
  `TEXT`, etc.) must be uppercase. The tokenizer does no case-normalization,
  and the parser compares tokens byte-for-byte against hardcoded lowercase
  keyword strings and uppercase type strings. Typing `SELECT` or `int` will
  fail to parse (`Unrecognized command` / `Unknown data type`).
- **Operators need surrounding whitespace.** Because the tokenizer only
  splits on whitespace and a fixed punctuation set (`( ) , ;`), writing
  `age>=18` with no spaces produces a single garbled token instead of three
  tokens. Always write `age >= 18`.
- **No OR, no grouping, no JOIN, no NULL, no DESC.** The `WHERE` grammar is a
  flat `AND`-chain only; there's no relational join across tables.
- **`data/` schema file format changed.** As of this bugfix pass, a column's
  `PRIMARY KEY`/`UNIQUE` flags are persisted on disk (previously they were
  silently dropped on save, meaning constraints were forgotten after every
  process restart — see the project's fix history). `.db`/`.meta` files
  created by a build prior to this fix will **misparse** under the current
  binary. Delete any old `data/` directory before using a build with this
  fix if you have leftover files from an earlier version.
- **`wal.log` is an audit trail, not a recovery mechanism.** Table durability
  comes entirely from direct file-backed row storage (each `.db` file is
  written on every insert/update/delete); `wal.log` records the same events
  as a plain-text log but isn't replayed on startup.
- **No transactions.** `BEGIN`/`COMMIT`/`ROLLBACK` don't exist in the
  grammar; every statement takes effect immediately and independently.
- **B+ tree deletion doesn't rebalance.** Deleting keys can leave underfull
  internal/leaf nodes; this doesn't cause incorrect results, but the tree
  isn't reclaiming/compacting space the way a production engine would.
- **Row-heap pages never reclaim tombstoned space.** Deleted/updated-and-grown
  rows leave a permanent gap in their page; the file only grows over time,
  it never shrinks or compacts.
- **A column can be `PRIMARY KEY` or `UNIQUE`, not stated as both** in a
  single column declaration (the parser's `if/else if` only lets one modifier
  apply per column).

---

## 7. Running the test suite

`test_bitbase.sh` is a black-box functional test suite (74 checks) that
drives the compiled binary exactly as a user would — piping SQL text into its
stdin and asserting on what it prints — covering every feature in
[§5](#5-sql-grammar-supported): meta commands, all `CREATE TABLE` variants,
`INSERT` (including PK/UNIQUE violations), every `WHERE` operator, `ORDER BY`,
`LIMIT`, `UPDATE`, `DELETE`, `DROP TABLE`, multi-table isolation, and —
importantly — that constraints and index-backed queries **survive a process
restart**.

### On Linux/macOS or Git Bash/WSL

```bash
./test_bitbase.sh path/to/bitbase_cli
```
e.g.
```bash
./test_bitbase.sh build/bitbase_cli
```

The script builds nothing itself — build first, then point it at the
resulting binary. It runs everything in a fresh temporary directory
(`mktemp -d`) so it never touches your real `data/`/`wal.log`, prints a
`PASS`/`FAIL` line per case with a diff-style dump of actual output on
failure, and exits non-zero if anything fails (safe to wire into CI).

### On native Windows PowerShell

The `.sh` script requires a POSIX shell and won't run directly under
`powershell.exe`. Either:
- run it from **Git Bash** or **WSL** instead (pointing at the `.exe`,
  e.g. `./test_bitbase.sh build/bitbase_cli/bitbase_cli.exe`), or
- use a native PowerShell port of the same 74 checks, if one has been added
  to this repo (check for a `test_bitbase.ps1` alongside the `.sh` file) —
  run it as:
  ```powershell
  ./test_bitbase.ps1 -Binary build\bitbase_cli\bitbase_cli.exe
  ```

### Interpreting failures

Every failure prints the exact expected substring and the actual captured
output, so a failing run is self-explanatory without needing to re-run
manually. A `FAIL` on any persistence/constraint test after a fresh clean
build most likely means the on-disk schema format and the binary have
drifted apart — see the `data/` format note in
[§6](#6-known-design-constraints).

---

## 8. Troubleshooting

**Nothing prints, no `data/` folder appears, when run from one shell but it
works fine from another (e.g. works in PowerShell, silent in Git Bash).**
This points to something shell/terminal-specific rather than a code bug —
first isolate whether it's a piping issue (try running the `.exe` completely
interactively, typing commands by hand, with no `|` involved) versus a
build/path issue (confirm you're pointing at the exact same freshly built
binary in both shells, e.g. via `Get-Item`/`ls -la` timestamps). If output
only ever appears in bursts or right before you'd expect a crash, suspect
output buffering in whatever terminal emulator you're using rather than the
program's logic — piped/non-console stdout can be fully buffered depending
on the environment.

**`Table already exists` / duplicate errors right after a clean rebuild.**
Delete `data/` and `wal.log` in your working directory before your first run
of a newly rebuilt binary, especially if you've rebuilt with schema-format
changes (see [§6](#6-known-design-constraints)) — stale files from an older
build will not parse correctly under a changed on-disk format.

**cmake not found.** Install it (see [§1](#1-quick-start)); there is no
build path in this project that avoids CMake other than manually invoking
`g++`/`clang++` on every `.cpp` file under `src/` with `-Iinclude -std=c++17`.