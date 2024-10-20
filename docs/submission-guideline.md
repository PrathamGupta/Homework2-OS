# **DUE:10/20/2024, 11:59PM**

## Submission Format:

### Programming Language Requirement:

- This assignment must be implemented in C/CPP. This is due to the low-level system calls needed to handle process creation and piping effectively, which are natively available in C/CPP.

### Extra Credit Tasks:

- Extra credit tasks require additional functionality and can be implemented using specific directives in the flow file. See explanation and examples of each at the end of Homework 2 Draft document.

### File Format: 

A \`.tar\` file named \`\<firstName\>\_\<lastName\>\_\<netID\>\_HW2.tar\`.

### Contents: 

The \`.tar\` file must contain the following files:  
  1\. partner.txt  
  2\. explanation.txt  
  3\. flow.c/cpp  
  4\. test\_cases.txt  
  5\. Any flow definition files used for testing (e.g., \`filecount.flow\`, \`complicated.flow\`)  
  6\. Any supporting files used in test cases, such as .txt files (e.g., foo.txt, result.txt)

### File Descriptions:

1\. partner.txt:  
   \- Contains a single line in the following format:  
       
     \<firstName\>\_\<lastName\>\_\<netID\>  
       
   \- For individual submissions, submit a blank \`partner.txt\` file.

2\. explanation.txt:  
   \- Provide an explanation of the design and thought process behind your implementation.  
   \- Suggested structure:  
     \- Node Implementation: How you implemented \`node\` for running a simple command.  
     \- Pipe Implementation: How you handled chaining two nodes using \`pipe\`.  
     \- Concatenation: How you implemented running a sequence of nodes using \`concatenate\`.  
     \- Extra Credit (if applicable): Mention any extra credit tasks implemented, such as \`error\` or \`file\` handling.  
   \- Mention any challenges faced or assumptions made during the development.

3\. flow.c:  
   \- The code implementing the interpreter that parses and executes flow description files.  
   \- Ensure that this file compiles and runs on Linux, Mac, or WSL environments.

4\. test\_cases.txt:  
   \- Include at least 5 test cases that demonstrate the functionality of your flow interpreter.  
   \- Test Case Structure:  
       
     `TEST-1`    
     `TEST-BEGIN`    
     `COMMAND: SHELL COMMAND`   
     `FLOW  COMMAND:`    
     `./flow <flowfile> <pipe_or_concatenation_to_execute>`    
     `EXPECTED OUTPUT:`    
     `<expected output>`    
     `TEST-END`  
     

     Example Test Case:  
       
`TEST-1`  
`TEST-BEGIN`  
`COMMAND:`  
`ls | wc`

`FLOW COMMAND:`  
`./flow filecount.flow doit`

`EXPECTED OUTPUT:`  
`<number of files, words, and characters in the current directory>`  
`TEST-END`

5\. Flow Definition Files:  
   \- Submit flow description files used in your test cases, such as \`filecount.flow\`, \`complicated.flow\`, etc.  
   \- Each file should define nodes, pipes, and any additional constructs used in the flow language.

6\. Supporting Files for Test Cases:

- Include all additional files required for your test cases, such as .txt files (foo.txt, result.txt) that contain data or text used as input in the test cases.  
- Each file should be named according to its function or content to avoid confusion.

## Example Test Case Guidelines:

### 1\. Basic Node and Pipe:

     
`TEST-1`  
`TEST-BEGIN`  
`COMMAND:`  
`ls | wc`

`FLOW COMMAND:`  
`./flow filecount.flow doit`

`EXPECTED OUTPUT:`  
`<number of files, words, and characters in the current directory>`  
`TEST-END`

### 2\. Concatenation of Outputs:

     
`TEST-2`  
`TEST-BEGIN`  
`COMMAND:`  
`(cat foo.txt; cat foo.txt | sed 's/o/u/g') | wc`

`FLOW COMMAND:`  
`./flow complicated.flow shenanigan`

`EXPECTED OUTPUT:`  
`<number of lines, words, and characters from the concatenated output of both commands>`  
`TEST-END`

### 3\. Extra Credit: Error Handling:

     
`TEST-3`  
`TEST-BEGIN`  
`COMMAND:`  
`mkdir a | wc`

`FLOW COMMAND:`  
`./flow mkdir_error.flow error_pipe`

`EXPECTED OUTPUT:`  
`<number of lines, words, and characters in the error message>`  
`TEST-END`

### 4\. Input and Output File Handling:

     
`TEST-4`  
`TEST-BEGIN`  
`COMMAND:`  
`wc < result.txt`

`FLOW COMMAND:`  
`./flow file_handling.flow read_file`

`EXPECTED OUTPUT:`  
`<word count of result.txt>`  
`TEST-END`

## Word of Caution:

\- Automated Grading: The submission will be graded automatically. Ensure all files are correctly named and formatted as per the guidelines.  
\- Misnamed or incorrectly formatted files may result in a zero for the assignment.

Additional Notes:  
\- Environment: Since this assignment doesn't require xv6 and allows the use of standard libraries, ensure that all programs can be compiled and executed using a Linux-based environment (Linux, WSL, Mac).  
\- Handling Flow Description Files: Make sure your code can read and parse flow description files correctly and execute the described actions in the right order.  
\- Pipes and Redirection: Pay attention to how pipes and redirection are implemented in your interpreter to ensure proper chaining of processes.