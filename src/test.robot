*** Settings ***
# Library         OperatingSystem
Library         Emulator.py
Test Teardown   Close All Connections

*** Test Cases ***
Establish A Connection
      [Documentation]   Merely makes a connection to an emulator
      [Tags]            smoke
      Open Connection

Make And Close A Connection
     [Documentation]    makes and closes a connection to an emulator
     Open Connection
     Close Connection

Check Boot Up Message
      [Documentation]   make a connection and read power-up message
      Open Connection
      ${output} =       Read Line
      Log               ${output}
      Should Match      ${output}   *ACORN ATOM*
      Close Connection
