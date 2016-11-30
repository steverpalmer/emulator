*** Settings ***
# Library         OperatingSystem
Library         Emulator.py
Test Teardown   Close All Connections

*** Test Cases ***
Establish A Connection
      [Documentation]   Merely makes a connection to an emulator
      [Tags]            smoke
      Open Connection
      Sleep             2      Settling Time, and time for something to go wrong

Make And Close A Connection
      [Documentation]    makes and closes a connection to an emulator
      [Tags]            smoke
      Open Connection
      Close Connection

Check Boot Up Message
      [Documentation]   make a connection and read power-up message
      [Tags]            smoke
      Open Connection
      ${output} =       Read Until Prompt
      Should Match      ${output}   *ACORN ATOM*
      Close Connection

Hello World
      [Documentation]   Check a simple command
      [Tags]            smoke
      Start Emulator
      ${result} =       Execute     P."HELLO WORLD"
      Should Be Equal   ${result}   HELLO WORLD
      Close Connection

Reset Test
      [Documentation]   Try reseting the Emulator
      Start Emulator
      Execute           P."RUNNING..."
      Reset
      ${result} =       Read Until Prompt
      Should Match      ${result}       *ACORN ATOM*
      Close Connection

*** Keywords ***
Start Emulator
      [Documentation]   Make the connection and check it is operating
      Open Connection
      ${bootmessage} =  Read Until Prompt
      Should Match      ${bootmessage}     *ACORN ATOM*

Execute
      [Arguments]       ${command}
      [Documentation]   run a command a return the result
      Write             ${command}
      ${output} =       Read Until Prompt
      [Return]          ${output}
