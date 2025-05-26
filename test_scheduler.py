import subprocess
import difflib
import os

# Paths
EXECUTABLE = "./ex3"
TEST_DIR = "CPU-Scheduler-Tests"
NUM_TESTS = 5
QUANTUM = "2"

def run_test(i):
    process_file = f"{TEST_DIR}/processes{i}.csv"
    expected_file = f"{TEST_DIR}/output{i}.txt"
    actual_output_file = f"{TEST_DIR}/my_output{i}.txt"

    try:
        with open(actual_output_file, "w") as outfile:
            subprocess.run(
                [EXECUTABLE, "CPU-Scheduler", process_file, QUANTUM],
                stdout=outfile,
                stderr=subprocess.PIPE,
                timeout=10  # Prevents hanging
            )
    except subprocess.TimeoutExpired:
        print(f"⏱️  Test {i} TIMED OUT (possible bug in scheduler)")
        return

    # Compare output
    with open(expected_file, "r") as f:
        expected = f.readlines()
    with open(actual_output_file, "r") as f:
        actual = f.readlines()

    diff = list(difflib.unified_diff(expected, actual, fromfile="expected", tofile="actual"))
    if not diff:
        print(f"✅ Test {i} PASSED")
    else:
        print(f"❌ Test {i} FAILED")
        for line in diff[:5]:
            print("   " + line.strip())

def main():
    if not os.path.exists(EXECUTABLE):
        print("❌ Error: ex3 executable not found. Please compile your code first.")
        return

    print("🧪 Running CPU Scheduler tests...\n")
    for i in range(1, NUM_TESTS + 1):
        run_test(i)

if __name__ == "__main__":
    main()
