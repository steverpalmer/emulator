*** Settings ***
Library                     OperatingSystem
Library                     Emulator.py
Test Teardown               Close All Connections

*** Variables ***
${BOOT MESSAGE}             *ACORN ATOM*
${DEFAULT_EXECUTE_TIMEOUT}  2.0
${FILENAME}                 ROBOT.BAS

*** Test Cases ***
Make And Close A Connection
      [Documentation]       Makes and Closes a connection to an emulator
      [Tags]                smoke
      Open Connection
      Sleep                 2   Settling Time, and time for something to go wrong
      Emulator Should Be Running
      Close Connection
      Emulator Should Be Stopped

Check Boot Up Message
      [Documentation]       make a connection and read power-up message
      [Tags]                smoke
      Open Connection
      ${output} =           Read Until Prompt
      Should Match          ${output}   ${BOOT MESSAGE}
      Close Connection

Hello World
      [Documentation]              Check a simple command
      [Tags]                       smoke
      Given emulator started
      When executing               P."HELLO WORLD"
      Then output should be        HELLO WORLD
      Close Connection

Reset Test 1
      [Documentation]              Try reseting the Emulator
      Given emulator started
      and executing                P."RUNNING..."
      When emulator is reset
      Then output should match     ${BOOT MESSAGE}
      Close Connection

Programming
      [Documentation]              Enter and run a program
      Given emulator started
      and executing                10 P."HELLO WORLD"
      and executing                20 E.
      when executing               RUN
      Then output should be        HELLO WORLD
      Close Connection

Reset Test 2
      [Documentation]              Reset clears the program
      Given emulator started
      and executing                10 P."HELLO WORLD"
      and executing                20 E.
      and emulator is reset
      When executing               LIST
      Then output should be        ${EMPTY}
      Close Connection

Syntax Error
      [Documentation]              Make a syntax error
      Given emulator started
      When executing               badcommand
      Then output should match     *ERROR 94*
      Close Connection

Save Test
      [Documentation]              Try and save a file
      Given emulator started
      and executing                10 P."HELLO WORLD"
      and executing                20 E.
      When executing               SAVE "${FILENAME}"      timeout=5
      Then output should be        ${EMPTY}
      and File Should Exist        ${FILENAME}
      Remove File                  ${FILENAME}
      Close Connection

Load Test and List
      [Documentation]              Try and load a file and list it
      Given emulator started
      and executing                10 P."HELLO WORLD"
      and executing                20 E.
      and executing                SAVE "${FILENAME}"      timeout=5
      and emulator is reset
      When executing               LOAD "${FILENAME}"      timeout=5
      and executing                LIST
      Then output should BE        ${SPACE * 3}10 P."HELLO WORLD"\n${SPACE * 3}20 E.\n
      Remove File                  ${FILENAME}
      Close Connection

Load Test and Run
      [Documentation]              Try and load a file and run it
      Given emulator started
      and executing                10 P."HELLO WORLD"
      and executing                20 E.
      and executing                SAVE "${FILENAME}"      timeout=5
      and emulator is reset
      When executing               LOAD "${FILENAME}"      timeout=5
      and executing                RUN
      Then output should be        HELLO WORLD
      Remove File                  ${FILENAME}
      Close Connection

*** Keywords ***
emulator started
      [Documentation]       Make the connection and check it is operating
      Open Connection
      ${bootmessage} =      Read Until Prompt
      Should Match          ${bootmessage}     ${BOOT MESSAGE}

executing
      [Arguments]           ${command}    ${timeout}=${DEFAULT_EXECUTE_TIMEOUT}
      [Documentation]       run a command a return the result
      Write                 ${command}
      Read Until Prompt     timeout=${timeout}

emulator is reset
      [Documentation]       Send reset to emulator and collect the bootmessage
      Reset
      Read Until Prompt
      
output should be
      [Arguments]           ${expected}
      ${actual} =           Read Result
      Should Be Equal       ${actual}    ${expected}

output should match
      [Arguments]           ${expected}
      ${actual}             Read Result
      Should Match          ${actual}    ${expected}
      