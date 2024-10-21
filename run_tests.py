import subprocess
import re

# Function to parse the test cases from the file
def parse_test_cases(file_name):
    test_cases = []
    with open(file_name, 'r') as file:
        content = file.read()

    # Split the content into individual tests
    raw_tests = re.split(r'TEST-\d+\s*TEST-BEGIN', content)
    for raw_test in raw_tests:
        if 'FLOW COMMAND:' in raw_test:
            command_match = re.search(r'COMMAND:\s*(.*)', raw_test, re.DOTALL)
            flow_command_match = re.search(r'FLOW COMMAND:\s*(.*)', raw_test, re.DOTALL)
            expected_output_match = re.search(r'EXPECTED OUTPUT:\s*(.*)\s*TEST-END', raw_test, re.DOTALL)

            if command_match and flow_command_match and expected_output_match:
                test_cases.append({
                    'shell_command': command_match.group(1).strip(),
                    'flow_command': flow_command_match.group(1).strip(),
                    'expected_output': expected_output_match.group(1).strip()
                })

    return test_cases

# Function to run a command and capture its output
def run_command(command):
    try:
        result = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        return result.stdout.strip()
    except Exception as e:
        return str(e)

# Function to run the tests
def run_tests(test_cases):
    for idx, test in enumerate(test_cases, 1):
        print(f"Running TEST-{idx}...")
        
        # Run the shell command and the flow command
        shell_output = run_command(test['shell_command'])
        flow_output = run_command(test['flow_command'])
        
        # Compare the output with the expected result
        if flow_output == test['expected_output']:
            print(f"TEST-{idx} PASSED")
        else:
            print(f"TEST-{idx} FAILED")
            print(f"Expected Output:\n{test['expected_output']}")
            print(f"Actual Output:\n{flow_output}")
        print("-" * 40)

if __name__ == "__main__":
    # Parse the test cases from test_cases.txt
    test_cases = parse_test_cases("test_cases.txt")
    
    # Run the parsed test cases
    run_tests(test_cases)

