import subprocess
import os
import shlex # For robust command splitting
import stat   # For os.chmod
import difflib # For showing differences

def run_test(input_file_path, expected_output_file_path, actual_outputs_folder):
    """
    Runs a single test case.
    Reads command and input from input_file_path, executes,
    and compares output with expected_output_file_path.
    Saves actual output to a file in actual_outputs_folder if the test fails.
    Returns True if test passes, False otherwise.
    """
    command_to_run = None
    script_input_lines = []
    command_found = False
    original_lines = []

    print(f"  Processing input file: {input_file_path}")
    try:
        with open(input_file_path, 'r') as f_in:
            original_lines = f_in.readlines()

        # Find the command and subsequent input
        for i, line_content in enumerate(original_lines):
            stripped_line = line_content.strip()

            if stripped_line.startswith("# ./") or \
                    stripped_line.startswith("./") or \
                    stripped_line.startswith("python ") or \
                    stripped_line.startswith("python3 ") or \
                    stripped_line.startswith("bash ") or \
                    stripped_line.startswith("sh "):

                command_str = stripped_line
                if stripped_line.startswith("# ./"):
                    command_str = stripped_line[len("# ./"):].strip()
                    command_str = f"./{command_str}"
                elif stripped_line.startswith("# python "):
                    command_str = stripped_line[len("# "):].strip()
                elif stripped_line.startswith("# python3 "):
                    command_str = stripped_line[len("# "):].strip()
                elif stripped_line.startswith("# bash "):
                    command_str = stripped_line[len("# "):].strip()
                elif stripped_line.startswith("# sh "):
                    command_str = stripped_line[len("# "):].strip()

                command_parts_temp = shlex.split(command_str)

                if command_parts_temp:
                    if not command_parts_temp[0].startswith(("./", "/", "python", "python3", "bash", "sh")) and \
                            (stripped_line.startswith("./") or stripped_line.startswith("# ./")):
                        command_parts_temp[0] = f"./{command_parts_temp[0]}"

                    command_to_run = command_parts_temp
                else:
                    print(f"  Error: Empty command string derived from line: {stripped_line}")
                    return False

                script_input_lines = [l.rstrip('\n') for l in original_lines[i+1:]]
                command_found = True
                print(f"  Found command: {' '.join(command_to_run)}")
                break

        if not command_found:
            print(f"  Error: Command not found in {input_file_path}")
            return False

        script_input_str = "\n".join(script_input_lines)
        if script_input_lines:
            script_input_str += "\n"

        printable_script_input = script_input_str.replace('\n', '\\n')
        print(f"  Input for command ('\\n' represents newline): '{printable_script_input}'")

        process = subprocess.run(
            command_to_run,
            input=script_input_str,
            capture_output=True,
            text=True,
            check=False
        )

        actual_output = process.stdout
        stderr_output = process.stderr

        if process.returncode != 0:
            print(f"  Warning: Script exited with code {process.returncode}")
            if stderr_output:
                print(f"  Stderr:\n------\n{stderr_output.strip()}\n------")

        print(f"  Reading expected output from: {expected_output_file_path}")
        with open(expected_output_file_path, 'r') as f_out:
            expected_output = f_out.read()

        original_actual_for_diff = actual_output
        original_expected_for_diff = expected_output

        normalized_actual = actual_output.strip()
        normalized_expected = expected_output.strip()

        if normalized_actual == normalized_expected:
            print(f"  Actual output matches expected output.")
            return True
        else:
            print(f"  OUTPUT MISMATCH!")

            # Save the actual output to a file
            if not os.path.exists(actual_outputs_folder):
                os.makedirs(actual_outputs_folder)
                print(f"  Created folder for actual outputs: {actual_outputs_folder}")

            # Construct a filename for the actual output, based on the input file's name
            base_input_filename = os.path.basename(input_file_path)
            actual_output_filename = f"actual_for_{base_input_filename}"
            actual_output_save_path = os.path.join(actual_outputs_folder, actual_output_filename)

            try:
                with open(actual_output_save_path, 'w') as f_actual_out:
                    f_actual_out.write(original_actual_for_diff)
                print(f"  --> Actual output saved to: {actual_output_save_path}")
            except Exception as e:
                print(f"  Error saving actual output to {actual_output_save_path}: {e}")

            print(f"  ------------------ EXPECTED (from {expected_output_file_path}) ------------------")
            print(original_expected_for_diff)
            print(f"  ------------------------------------------------------------")
            print(f"  ------------------ ACTUAL (from {' '.join(command_to_run)}) --------------------")
            print(original_actual_for_diff)
            print(f"  ------------------------------------------------------------")

            print(f"  ------------------ DETAILED DIFF (unified format) ----------")
            expected_lines = original_expected_for_diff.splitlines(keepends=True)
            actual_lines = original_actual_for_diff.splitlines(keepends=True)

            diff = difflib.unified_diff(
                expected_lines,
                actual_lines,
                fromfile=f"Expected: {expected_output_file_path}",
                tofile=f"Actual (also saved to {actual_output_save_path})",
                lineterm='\n'
            )

            diff_output_list = list(diff)
            if not diff_output_list:
                print("    No textual differences found by difflib.unified_diff (might be trailing whitespace differences only).")
            else:
                for line in diff_output_list:
                    print(f"    {line.rstrip()}")
            print(f"  ------------------------------------------------------------")
            return False

    except FileNotFoundError:
        print(f"  Error: A required file was not found. Ensure {input_file_path} and {expected_output_file_path} exist.")
        return False
    except PermissionError as e:
        script_name = command_to_run[0] if command_to_run else "Unknown script"
        print(f"  Error: Permission denied for script: {script_name}. Details: {e}")
        if command_to_run and os.path.exists(command_to_run[0]) and command_to_run[0].startswith("./"):
            try:
                print(f"  Attempting to make {command_to_run[0]} executable...")
                current_permissions = stat.S_IMODE(os.lstat(command_to_run[0]).st_mode)
                os.chmod(command_to_run[0], current_permissions | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
                print(f"  Permissions updated for {command_to_run[0]}. Please try running the test again.")
            except Exception as chmod_e:
                print(f"  Failed to change permissions for {command_to_run[0]}: {chmod_e}")
        return False
    except Exception as e:
        print(f"  An unexpected error occurred: {e}")
        return False

def main():
    num_sets = 3
    test_folder = "Focus-Mode-Tests"
    actual_outputs_folder = "Actual-Outputs_Part1" # Folder to save actual outputs of failed tests

    # Create the actual_outputs_folder if it doesn't exist at the start
    if not os.path.exists(actual_outputs_folder):
        try:
            os.makedirs(actual_outputs_folder)
            print(f"Created directory for storing actual outputs of failed tests: ./{actual_outputs_folder}")
        except OSError as e:
            print(f"Error creating directory {actual_outputs_folder}: {e}. Failed outputs might not be saved.")
            # Optionally, you could decide to exit or handle this more gracefully
            # For now, we'll let it continue and run_test will try to create it again if needed.

    passed_all = True
    tests_run = 0
    tests_passed = 0

    print("====================")
    print(f"STARTING TEST RUNNER (tests in ./{test_folder}/)")
    print("====================")

    for i in range(1, num_sets + 1):
        input_file = os.path.join(test_folder, f"input{i}.txt")
        output_file = os.path.join(test_folder, f"output{i}.txt")

        print(f"\n--- Test Set {i}: {input_file} vs {output_file} ---")

        if not os.path.exists(input_file):
            print(f"  Error: Input file {input_file} not found. Skipping test set {i}.")
            passed_all = False
            continue
        if not os.path.exists(output_file):
            print(f"  Error: Output file {output_file} not found. Skipping test set {i}.")
            passed_all = False
            continue

        tests_run += 1
        # Pass the actual_outputs_folder to the run_test function
        result = run_test(input_file, output_file, actual_outputs_folder)
        if result:
            print(f"Test Set {i}: PASS")
            tests_passed +=1
        else:
            print(f"Test Set {i}: FAIL")
            passed_all = False
        print("-----------------------------------")

    print("\n====================")
    print("TESTING COMPLETE")
    if tests_run > 0:
        print(f"Summary: {tests_passed}/{tests_run} tests passed.")
        if passed_all:
            print("All tests passed!")
        else:
            print(f"Some tests failed. Check the '{actual_outputs_folder}' directory for outputs of failed tests.")
    else:
        print(f"No tests were run. Ensure 'inputN.txt' and 'outputN.txt' files exist in the '{test_folder}' directory.")
    print("====================")

if __name__ == "__main__":
    main()
