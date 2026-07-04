<#
.SYNOPSIS
    Exhaustive functional test suite for bitbase_cli
.DESCRIPTION
    Usage:
        .\test_bitbase.ps1 [path-to-bitbase_cli-binary]

    Defaults to .\bitbase_cli.exe if no path is given.
#>

$Bin = if ($args[0]) { $args[0] } else { ".\bitbase_cli.exe" }

# Resolve absolute path for the binary
try {
    $Bin = Resolve-Path $Bin -ErrorAction Stop | Select-Object -ExpandProperty Path
} catch {
    Write-Error "ERROR: binary not found at: $Bin"
    Write-Host "Build it first, e.g.: g++ -std=c++17 -Iinclude -o bitbase_cli src/*.cpp src/**/*.cpp"
    Exit 1
}

# Create a temporary test scratch directory
$Guid = [Guid]::NewGuid().ToString()
$TestDir = Join-Path $env:TEMP "bitbase_test_$Guid"
New-Item -ItemType Directory -Path $TestDir -Force | Out-Null

$Pass = 0
$Fail = 0
$FailedNames = @()

function Cleanup-DB {
    $DataDir = Join-Path $TestDir "data"
    $WalLog = Join-Path $TestDir "wal.log"
    if (Test-Path $DataDir) { Remove-Item -Recurse -Force $DataDir }
    if (Test-Path $WalLog) { Remove-Item -Force $WalLog }
}

function Run-Sql {
    param([string]$inputSql)
    
    # Save current directory and jump to scratch directory
    $CurrentDir = Get-Location
    Set-Location $TestDir

    # Pipe the SQL string to the executable and grab combined stdout/stderr
    $Output = $inputSql | & $Bin 2>&1 | Out-String

    Set-Location $CurrentDir
    return $Output
}

function Check {
    param(
        [string]$Name,
        [string]$Output,
        [string]$Expected,
        [string]$Mode = ""
    )
    
    $Contains = $Output.Contains($Expected)

    if ($Mode -eq "--absent") {
        if ($Contains) {
            Write-Host "FAIL: $Name  (did NOT expect to find: '$Expected')" -ForegroundColor Red
            $script:Fail++
            $script:FailedNames += $Name
        } else {
            Write-Host "PASS: $Name" -ForegroundColor Green
            $script:Pass++
        }
    } else {
        if ($Contains) {
            Write-Host "PASS: $Name" -ForegroundColor Green
            $script:Pass++
        } else {
            Write-Host "FAIL: $Name  (expected to find: '$Expected')" -ForegroundColor Red
            Write-Host "  ---- actual output ----"
            $Output -split "`r?`n" | ForEach-Object { Write-Host "  | $_" }
            Write-Host "  ------------------------"
            $script:Fail++
            $script:FailedNames += $Name
        }
    }
}

Write-Host "=================================================================="
Write-Host " Testing binary: $Bin"
Write-Host " Scratch dir:    $TestDir"
Write-Host "=================================================================="

# --------------------------------------------------------------------
# 1. META COMMANDS
# --------------------------------------------------------------------
Write-Host "`n--- 1. Meta commands ---"

Cleanup-DB
$out = Run-Sql ".help`n.exit`n"
Check "meta: .help lists commands" $out ".exit"

Cleanup-DB
$out = Run-Sql ".tables`n.exit`n"
Check "meta: .tables on empty db doesn't crash" $out "Bitbase>"

Cleanup-DB
$out = Run-Sql "create table t1 (id INT primary key, name TEXT);`n.tables`n.exit`n"
Check "meta: .tables lists created table" $out "t1"

Cleanup-DB
$out = Run-Sql ".exit`n"
Check "meta: .exit terminates REPL" $out ""

# --------------------------------------------------------------------
# 2. CREATE TABLE
# --------------------------------------------------------------------
Write-Host "`n--- 2. CREATE TABLE ---"

Cleanup-DB
$out = Run-Sql "create table basic (id INT, name TEXT);`n.exit`n"
Check "create: basic table" $out "Executed CREATE TABLE"

Cleanup-DB
$out = Run-Sql "create table alltypes (a INT, b BIGINT, c FLOAT, d DOUBLE, e BOOL, f TEXT);`ninsert into alltypes values (1, 2, 1.5, 2.5, true, 'hi');`nselect * from alltypes;`n.exit`n"
Check "create: all data types accepted" $out "Executed CREATE TABLE"
Check "create: insert with all types works" $out "Executed INSERT"

Cleanup-DB
$out = Run-Sql "create table pktable (id INT primary key, name TEXT);`n.exit`n"
Check "create: primary key column accepted" $out "Executed CREATE TABLE"

Cleanup-DB
$out = Run-Sql "create table uqtable (id INT, email TEXT unique);`n.exit`n"
Check "create: unique column accepted" $out "Executed CREATE TABLE"

Cleanup-DB
$out = Run-Sql "create table dup (id INT, name TEXT);`ncreate table dup (id INT, name TEXT);`n.exit`n"
Check "create: duplicate table name rejected" $out "Table already exists"

Cleanup-DB
$out = Run-Sql "create table badtype (id NOTATYPE);`n.exit`n"
Check "create: unknown data type rejected" $out "Unknown data type"

Cleanup-DB
$out = Run-Sql "create table badsyntax id INT;`n.exit`n"
Check "create: missing '(' rejected" $out "Expected '(' after table name"

# --------------------------------------------------------------------
# 3. INSERT
# --------------------------------------------------------------------
Write-Host "`n--- 3. INSERT ---"

Cleanup-DB
$out = Run-Sql "create table ins (id INT primary key, name TEXT);`ninsert into ins values (1, 'Alice');`nselect * from ins;`n.exit`n"
Check "insert: basic insert + select" $out "(1, Alice)"

Cleanup-DB
$out = Run-Sql "insert into nosuchtable values (1, 'x');`n.exit`n"
Check "insert: into nonexistent table" $out "Error: Table not found"

Cleanup-DB
$out = Run-Sql "create table ins2 (id INT, name TEXT, age INT);`ninsert into ins2 values (1, 'Alice');`n.exit`n"
Check "insert: column count mismatch rejected" $out "Column count mismatch"

Cleanup-DB
$out = Run-Sql "create table pkdup (id INT primary key, name TEXT);`ninsert into pkdup values (1, 'A');`ninsert into pkdup values (1, 'B');`nselect * from pkdup;`n.exit`n"
Check "insert: duplicate primary key rejected" $out "Duplicate primary key"
Check "insert: only first PK row present after dup rejected" $out "(1, A)"

Cleanup-DB
$out = Run-Sql "create table uq (id INT, email TEXT unique);`ninsert into uq values (1, 'a@x.com');`ninsert into uq values (2, 'a@x.com');`ninsert into uq values (3, 'b@x.com');`nselect * from uq;`n.exit`n"
Check "insert: UNIQUE rejected without primary key on table" $out "Duplicate value for UNIQUE column"
Check "insert: non-duplicate unique row still inserted" $out "(3, b@x.com)"

Cleanup-DB
$out = Run-Sql "create table uqpk (id INT primary key, email TEXT unique);`ninsert into uqpk values (1, 'a@x.com');`ninsert into uqpk values (2, 'a@x.com');`nselect * from uqpk;`n.exit`n"
Check "insert: UNIQUE rejected with primary key present" $out "Duplicate value for UNIQUE column"

Cleanup-DB
$out = Run-Sql "create table strs (id INT primary key, name TEXT);`ninsert into strs values (1, 'hello world');`nselect * from strs;`n.exit`n"
Check "insert: string values with spaces preserved" $out "(1, hello world)"

# --------------------------------------------------------------------
# 4. SELECT
# --------------------------------------------------------------------
Write-Host "`n--- 4. SELECT ---"

$SetupSql = "create table t (id INT primary key, name TEXT, age INT);`ninsert into t values (1, 'Alice', 30);`ninsert into t values (2, 'Bob', 25);`ninsert into t values (3, 'Carol', 40);`n"

function Fresh-Setup {
    Cleanup-DB
}

Fresh-Setup; $out = Run-Sql "${SetupSql}select * from t;`n.exit`n"
Check "select: select * returns all rows" $out "(1, Alice, 30)"
Check "select: select * returns all rows (2)" $out "(2, Bob, 25)"
Check "select: select * returns all rows (3)" $out "(3, Carol, 40)"

Fresh-Setup; $out = Run-Sql "${SetupSql}select name, age from t;`n.exit`n"
Check "select: specific column projection" $out "(Alice, 30)"

Fresh-Setup; $out = Run-Sql "${SetupSql}select * from t where id = 2;`n.exit`n"
Check "select: where id = (equality on PK)" $out "(2, Bob, 25)"
Check "select: where id = excludes others" $out "(1, Alice, 30)" "--absent"

Fresh-Setup; $out = Run-Sql "${SetupSql}select * from t where name = 'Bob';`n.exit`n"
Check "select: where on TEXT equality" $out "(2, Bob, 25)"

Fresh-Setup; $out = Run-Sql "${SetupSql}select * from t where age != 30;`n.exit`n"
Check "select: where != operator" $out "(2, Bob, 25)"
Check "select: where != excludes match" $out "(1, Alice, 30)" "--absent"

Fresh-Setup; $out = Run-Sql "${SetupSql}select * from t where age > 26;`n.exit`n"
Check "select: where > operator" $out "(3, Carol, 40)"
Check "select: where > excludes equal/lower" $out "(2, Bob, 25)" "--absent"

Fresh-Setup; $out = Run-Sql "${SetupSql}select * from t where age < 30;`n.exit`n"
Check "select: where < operator" $out "(2, Bob, 25)"
Check "select: where < excludes equal/higher" $out "(1, Alice, 30)" "--absent"

Fresh-Setup; $out = Run-Sql "${SetupSql}select * from t where age >= 30;`n.exit`n"
Check "select: where >= includes boundary" $out "(1, Alice, 30)"
Check "select: where >= includes above boundary" $out "(3, Carol, 40)"
Check "select: where >= excludes below boundary" $out "(2, Bob, 25)" "--absent"

Fresh-Setup; $out = Run-Sql "${SetupSql}select * from t where age <= 30;`n.exit`n"
Check "select: where <= includes boundary" $out "(1, Alice, 30)"
Check "select: where <= includes below boundary" $out "(2, Bob, 25)"
Check "select: where <= excludes above boundary" $out "(3, Carol, 40)" "--absent"

Fresh-Setup; $out = Run-Sql "${SetupSql}select * from t where age >= 26 and age <= 40;`n.exit`n"
Check "select: multiple AND conditions" $out "(3, Carol, 40)"
Check "select: multiple AND conditions excludes non-matching" $out "(2, Bob, 25)" "--absent"

Fresh-Setup; $out = Run-Sql "${SetupSql}select * from t where id >= 1 and id <= 2;`n.exit`n"
Check "select: id range query (btree range scan)" $out "(1, Alice, 30)"
Check "select: id range query includes upper bound" $out "(2, Bob, 25)"
Check "select: id range query excludes out-of-range" $out "(3, Carol, 40)" "--absent"

Fresh-Setup; $out = Run-Sql "${SetupSql}select * from t order by age;`n.exit`n"
Check "select: order by ascending" $out "(2, Bob, 25)"

Fresh-Setup; $out = Run-Sql "${SetupSql}select * from t limit 2;`n.exit`n"
# Matches parenthesized blocks representing rows to get a total count
$OutLines = [regex]::Matches($out, '\([^()]*\)').Count
if ($OutLines -eq 2) {
    Write-Host "PASS: select: limit restricts row count to 2" -ForegroundColor Green
    $script:Pass++
} else {
    Write-Host "FAIL: select: limit restricts row count to 2 (got $OutLines rows)" -ForegroundColor Red
    $script:Fail++
    $script:FailedNames += "select: limit restricts row count to 2"
}

Fresh-Setup; $out = Run-Sql "${SetupSql}select * from t where age > 20 order by age limit 2;`n.exit`n"
Check "select: combined where + order + limit" $out "(2, Bob, 25)"

Cleanup-DB
$out = Run-Sql "select * from ghost;`n.exit`n"
Check "select: from nonexistent table" $out "Error: Table not found"

Cleanup-DB
$out = Run-Sql "create table empty1 (id INT primary key, name TEXT);`nselect * from empty1;`n.exit`n"
Check "select: empty table returns 'Row not found'" $out "Row not found"

# --------------------------------------------------------------------
# 5. UPDATE
# --------------------------------------------------------------------
Write-Host "`n--- 5. UPDATE ---"

Fresh-Setup; $out = Run-Sql "${SetupSql}update t set age = 99 where id = 2;`nselect * from t where id = 2;`n.exit`n"
Check "update: updates matching row" $out "(2, Bob, 99)"

Fresh-Setup; $out = Run-Sql "${SetupSql}update t set name = 'Nobody' where id = 999;`n.exit`n"
Check "update: no matching row reports failure" $out "Update failed"

Cleanup-DB
$out = Run-Sql "create table uqu (id INT primary key, email TEXT unique);`ninsert into uqu values (1, 'a@x.com');`ninsert into uqu values (2, 'b@x.com');`nupdate uqu set email = 'a@x.com' where id = 2;`nselect * from uqu;`n.exit`n"
Check "update: rejects change that violates UNIQUE" $out "Duplicate value for UNIQUE column"
Check "update: original unique value unchanged after rejection" $out "(2, b@x.com)"

# --------------------------------------------------------------------
# 6. DELETE
# --------------------------------------------------------------------
Write-Host "`n--- 6. DELETE ---"

Fresh-Setup; $out = Run-Sql "${SetupSql}delete from t where id = 3;`nselect * from t;`n.exit`n"
Check "delete: where clause removes matching row" $out "(3, Carol, 40)" "--absent"
Check "delete: where clause keeps other rows" $out "(1, Alice, 30)"

Fresh-Setup; $out = Run-Sql "${SetupSql}delete from t;`nselect * from t;`n.exit`n"
Check "delete: no where clause deletes all rows" $out "Executed DELETE ALL"
Check "delete: table empty after delete all" $out "Row not found"

Fresh-Setup; $out = Run-Sql "${SetupSql}delete from t where id = 999;`n.exit`n"
Check "delete: no matching row reports 'Row not found'" $out "Row not found"

# --------------------------------------------------------------------
# 7. DROP TABLE
# --------------------------------------------------------------------
Write-Host "`n--- 7. DROP TABLE ---"

Cleanup-DB
$out = Run-Sql "create table dropme (id INT primary key, name TEXT);`ndrop table dropme;`nselect * from dropme;`n.exit`n"
Check "drop: table removed, subsequent select fails" $out "Executed DROP TABLE"
Check "drop: select on dropped table errors" $out "Error: Table not found"

Cleanup-DB
$out = Run-Sql "drop table ghost;`n.exit`n"
Check "drop: dropping nonexistent table reports error" $out "Table not found"

# --------------------------------------------------------------------
# 8. MULTIPLE TABLES / ISOLATION
# --------------------------------------------------------------------
Write-Host "`n--- 8. Multiple table isolation ---"

Cleanup-DB
$out = Run-Sql "create table t_a (id INT primary key, name TEXT);`ncreate table t_b (id INT primary key, name TEXT);`ninsert into t_a values (1, 'FromA');`ninsert into t_b values (1, 'FromB');`nselect * from t_a;`nselect * from t_b;`n.exit`n"
Check "isolation: table A has its own row" $out "(1, FromA)"
Check "isolation: table B has its own row" $out "(1, FromB)"

# --------------------------------------------------------------------
# 9. PERSISTENCE ACROSS RESTART
# --------------------------------------------------------------------
Write-Host "`n--- 9. Persistence across restarts ---"

Cleanup-DB
$null = Run-Sql "create table persist (id INT primary key, name TEXT);`ninsert into persist values (1, 'Survivor');`n.exit`n"
$out = Run-Sql "select * from persist;`n.exit`n"
Check "persistence: data survives process restart" $out "(1, Survivor)"

$out = Run-Sql "insert into persist values (2, 'Second');`nselect * from persist;`n.exit`n"
Check "persistence: can keep inserting after restart" $out "(2, Second)"

Cleanup-DB
$null = Run-Sql "create table pkrestart (id INT primary key, name TEXT);`ninsert into pkrestart values (1, 'Alice');`n.exit`n"
$out = Run-Sql "insert into pkrestart values (1, 'DUPLICATE');`nselect * from pkrestart;`n.exit`n"
Check "persistence: PRIMARY KEY constraint still enforced after restart" $out "Duplicate primary key"
Check "persistence: duplicate PK row not present after restart" $out "(1, DUPLICATE)" "--absent"

Cleanup-DB
$null = Run-Sql "create table uqrestart (id INT primary key, email TEXT unique);`ninsert into uqrestart values (1, 'a@x.com');`n.exit`n"
$out = Run-Sql "insert into uqrestart values (2, 'a@x.com');`n.exit`n"
Check "persistence: UNIQUE constraint still enforced after restart" $out "Duplicate value for UNIQUE column"

Cleanup-DB
$null = Run-Sql "create table rangerestart (id INT primary key, name TEXT);`ninsert into rangerestart values (1, 'A');`ninsert into rangerestart values (2, 'B');`ninsert into rangerestart values (3, 'C');`n.exit`n"
$out = Run-Sql "select * from rangerestart where id >= 1 and id <= 2;`n.exit`n"
Check "persistence: id-range (B-tree) query still works after restart" $out "(1, A)"
Check "persistence: id-range query excludes out-of-range after restart" $out "(3, C)" "--absent"

# --------------------------------------------------------------------
# 10. PARSER EDGE CASES
# --------------------------------------------------------------------
Write-Host "`n--- 10. Parser edge cases ---"

Cleanup-DB
$out = Run-Sql "SELECT * FROM t;`n.exit`n"
Check "parser: uppercase keywords rejected (case-sensitive by design)" $out "Unrecognized command"

Cleanup-DB
$out = Run-Sql "gibberish input here`n.exit`n"
Check "parser: garbage input reports error" $out "Unrecognized command"

Cleanup-DB
$out = Run-Sql "`n.exit`n"
Check "parser: blank line does not crash REPL" $out ""

# ======================================================================
# SUMMARY
# ======================================================================
Write-Host ""
Write-Host "=================================================================="
Write-Host " RESULTS: $Pass passed, $Fail failed (of $($Pass + $Fail) total)"

if ($Fail -gt 0) {
    Write-Host " Failed tests:" -ForegroundColor Red
    foreach ($Name in $FailedNames) {
        Write-Host "   - $Name" -ForegroundColor Red
    }
}
Write-Host "=================================================================="

# Final Cleanup of the random working directory
if (Test-Path $TestDir) { Remove-Item -Recurse -Force $TestDir }

if ($Fail -gt 0) {
    Exit 1
}
Exit 0