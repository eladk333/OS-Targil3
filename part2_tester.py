import os
import subprocess
import filecmp
import shutil
import glob
import tempfile

# Constants
TEST_DIR = "CPU-Scheduler-Tests"
EXECUTABLE = "./ex3"
OUTPUT_DIR = "test-outputs"

os.makedirs(OUTPUT_DIR, exist_ok=True)  # Ensure output dir exists

def extract_command_and_data(csv_path):
    with open(csv_path, 'r') as f:
        lines = f.readlines()

    comments = [line.strip() for line in lines if line.strip().startswith("#")]
    command = comments[-1][2:].strip() if comments else None  # Get last comment and strip "# "

    data_lines = [line for line in lines if not line.strip().startswith("#")]
    return command, data_lines

def run_test(csv_file):
    base_name = os.path.basename(csv_file)
    test_num = ''.join(filter(str.isdigit, base_name))  # e.g. processes1.csv -> 1
    expected_output = os.path.join(TEST_DIR, f"output{test_num}.txt")
    actual_output = os.path.join(OUTPUT_DIR, f"actual-output{test_num}.txt")

    command, data_lines = extract_command_and_data(csv_file)
    if not command:
        print(f"Test {test_num}: FAILED (no command found)")
        return

    with tempfile.TemporaryDirectory() as temp_dir:
        temp_csv_path = os.path.join(temp_dir, f"processes{test_num}.csv")

        # Write cleaned CSV
        with open(temp_csv_path, 'w') as f:
            f.writelines(data_lines)

        # Modify the command to use the cleaned CSV path
        modified_command = command.replace(f"{TEST_DIR}/processes{test_num}.csv", temp_csv_path)

        try:
            with open(actual_output, 'w') as out_file:
                subprocess.run(modified_command.split(), stdout=out_file, stderr=subprocess.STDOUT, check=True)
        except subprocess.CalledProcessError:
            print(f"Test {test_num}: FAILED (execution error)")
            return

        # Compare with expected output
        if filecmp.cmp(actual_output, expected_output, shallow=False):
            print(f"Test {test_num}: PASSED")
        else:
            print(f"Test {test_num}: FAILED")

def main():
    csv_files = sorted(glob.glob(os.path.join(TEST_DIR, "processes*.csv")))
    for csv_file in csv_files:
        run_test(csv_file)

if __name__ == "__main__":
    main()
