*** Settings ***
Library                     Emulator.py
Test Teardown               Close All Connections

*** Variables ***
${BOOT MESSAGE}             *ACORN ATOM*
${TEST MESSAGE}             HELLO WORLD
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
      [Documentation]       Check a simple command
      [Tags]                smoke
      Start Emulator
      ${result} =           Execute     P."${TEST MESSAGE}"
      Should Be Equal       ${result}   ${TEST MESSAGE}
      Close Connection

Reset Test
      [Documentation]       Try reseting the Emulator
      Start Emulator
      Execute               P."RUNNING..."
      Reset
      ${result} =           Read Until Prompt
      Should Match          ${result}   ${BOOT MESSAGE}
      Close Connection

Load Test
      [Documentation]       Try and load a file
      Start Emulator
      Execute               LOAD "ATAP4.1.BAS"    timeout=5
      ${result} =           Execute     RUN
      Should Be Equal       ${result}   A PROGRAM!
      Close Connection

*** Keywords ***
Start Emulator
      [Documentation]       Make the connection and check it is operating
      Open Connection
      ${bootmessage} =      Read Until Prompt
      Should Match          ${bootmessage}     ${BOOT MESSAGE}

Execute
      [Arguments]           ${command}    ${timeout}=${DEFAULT_EXECUTE_TIMEOUT}
      [Documentation]       run a command a return the result
      Write                 ${command}
      ${output} =           Read Until Prompt    timeout=${timeout}
      [Return]              ${output}
