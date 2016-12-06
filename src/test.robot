*** Settings ***
Library                     Emulator.py
Test Teardown               Close All Connections

*** Variables ***
${BOOT MESSAGE}             *ACORN ATOM*
${DEFAULT_EXECUTE_TIMEOUT}  2.0

*** Test Cases ***
Configuration Report
      [Documentation]       Records some configuration data
      Log Executable Name
      Log Configuration File

Make And Close A Connection
      [Documentation]       Makes and Closes a connection to an emulator
      [Tags]                smoke
      Open Connection
      Sleep                 2   Settling Time, and time for something to go wrong
      Close Connection

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

Reset Test
      [Documentation]              Try reseting the Emulator
      Given emulator started
      When executing               P."RUNNING..."
      and emulator is reset
      Then output should match     ${BOOT MESSAGE}
      Close Connection

Load Test
      [Documentation]              Try and load a file
      Given emulator started
      When executing               LOAD "ATAP4.1.BAS"    timeout=5
      and executing                RUN
      Then output should be        A PROGRAM!
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
      