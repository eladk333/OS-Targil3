import subprocess
import difflib
import os

# Paths and configuration
EXECUTABLE = "./ex3"
TEST_DIR = "CPU-Scheduler-Tests"
NUM_TESTS = 5
QUANTUM = "2"
DIFF_DIR = os.path.join(TEST_DIR, "diffs")

def run_test(i):
    process_file = f"{TEST_DIR}/processes{i}.csv"
    expected_file = f"{TEST_DIR}/output{i}.txt"
    actual_output_file = f"{TEST_DIR}/my_output{i}.txt"
    diff_output_file = os.path.join(DIFF_DIR, f"diff{i}.txt")

    try:
        with open(actual_output_file, "w") as outfile:
            subprocess.run(
                [EXECUTABLE, "CPU-Scheduler", process_file, QUANTUM],
                stdout=outfile,
                stderr=subprocess.PIPE,
                timeout=60
            )
    except subprocess.TimeoutExpired:
        print(f"⏱️  Test {i} TIMED OUT (possible bug in scheduler)")
        return

    # Load and normalize output lines
    with open(expected_file, "r") as f:
        expected = [line.rstrip() for line in f.readlines()]
    with open(actual_output_file, "r") as f:
        actual = [line.rstrip() for line in f.readlines()]

    # Diff and display
    diff = list(difflib.unified_diff(expected, actual, fromfile="expected", tofile="actual", lineterm=''))
    if not diff:
        print(f"✅ Test {i} PASSED")
    else:
        print(f"❌ Test {i} FAILED (showing first few lines of diff)")
        for line in diff[:10]:
            print("   " + line)

        # Save full diff
        with open(diff_output_file, "w") as f:
            f.write("\n".join(diff))

def main():
    if not os.path.exists(EXECUTABLE):
        print("❌ Error: ex3 executable not found. Please compile your code first.")
        return

    os.makedirs(DIFF_DIR, exist_ok=True)

    print("🧪 Running CPU Scheduler tests...\n")
    for i in range(1, NUM_TESTS + 1):
        run_test(i)

if __name__ == "__main__":
    main()
