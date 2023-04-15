  Gate sequencer by Plop

  Update 17 FEB 2021

  64 step sequencer, 8 parts, multiple play mode, eurorack format.

  ________________________________________________________________________________________________
  ________________________________________________________________________________________________
  ________________________________________________________________________________________________

  TODO :

    SOFTWARE

      -Clock multiplier/divider per part
      -Detect clock internal/external (switching jack)
      -Reset -> interupt
      -Patern choice
      -Save/Load (SD card)
  

    HARDWARE

      -Use faster microcontroler (blue pills or teensy ?)
      -Add switching jack for clock
      -Add SD card
      -12v alimentation
      -"transistorisation" des entrées/sorties
      
      
    SCHEMATIC & PCB
    
      -Resistor value for LED & screen
      -Routing Main_Board
      -Drawing CPU_Board


  DONE :

    -Tempo
    -Screen driver
    -LED driver
    -Key matrix
    -Part choice
    -Number of step
    -Pages navigation
    -Step ON-OFF
    -Play mode
    -Fill/Clear
    -Copy/Past
    -Pause/Play/Stop
    -Gate size
    -Step jump
    -Reset
    -Internal clock

  ________________________________________________________________________________________________
  ________________________________________________________________________________________________
  ________________________________________________________________________________________________


  FUNCTIONS :
  

    Buttons =
        -Step on/off
        -Shift + step button = step jump
        -Fill
        -Fill + shift = Clear
        -Copy/Paste
        -Copy/Paste + shift = Save
        -Pause/Play
        -Pause/Play + shift = Stop
    

    Encoders =
        -Tempo
        -Tempo + push = clock divider
        -Gate length
        -Gate length + push = Number of step
        -Page navigation
        -Page navigation + push = Play mode
    

    Selector =
        -Part choice

    Play mode =
        -Forward
        -Backward
        -Ping Pong
        -Random
        -Brownien
    

  --------------------------------------------------------------------------
