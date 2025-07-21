Software is free to use and share for non-profit purposes. 
Improvements and suggestions are welcome. Would be great with a new mode having 2 emulated cursors maybe, one that moves with stick, and one that only search bmps and highlights the coordinates found(on multiple targets it will shift to next target on repeated search(already implemented))

Description:
A dll/plugin that Emulates mouse input with an Xinput controller. Coordinates are decided by presaved .BMP 24 bit format files or an emulated cursor or both.  
When a button is pressed on the controller, the game window content will be copied to memory. 
Then the presaved bmp will be compared to each pixel position until it fits, similar to a puzzle.
If the presaved bmp do fit in a location, a (mouse click/emulated cursor move/or both) command (with SendInput or PostMessage) will be sent to the coordinates found.

3 modes:
0 = no emulated cursor, only bmp searches
1 = emulated cursor + bmp search(controllable with left or right stick depending on setting)                
2 = button mapping mode. will map button presses(X, Y, A, B, R2, L2, L3, R3 buttons) to emulated cursor position      

Hooks:
ClipCursorHook=1 //cursor trap to window returned without call
GetKeystateHook=1 //used to read keys. not tested
GetAsynckeystateHook=1 //used to read keys. not tested
GetCursorposHook=1 // returns emulated cursor position
SetCursorHook=1 // sets emulated cursor position



guide:
1: Start a game with this dll.
1: Open Setting file named Xinput.ini ( was named screenshotinput.ini had to rename as it was too long)
2: Setting AllowModeChange lets you change modes with "start" button on controller
3: Set to "edit mode" to start mapping, saving bmps. 
4: In Setting button InputType, lets you decide if the mapped button shall just move cursor to coordinate just press or do both. multiple bmp matches supported
5: Setting UseRealMouse lets you decide to use system mouse with SendInput. or use PostMessage to attached game window
5: Enjoy.

Mapping:
Arrow keys = move getcursorposhook to window edge. Scrolls maps on strategy games
Left trigger = Right mouse button
Right trigger = Left mouse button
Right joy axis = mousemove ( if setting Righthanded is 1, else its the left joy axis)
X, Y, A, B, R2, L2, L3, R3 can be mapped to bmp
No map to keyboard yet


Settings: Xinput.ini

//   1 = emulated cursor with red dot + bmp search(controllable with left stick)                //
//   2 = button mapping mode. will map button presses(A,B,X,Y buttons) to red dot position      //
//                                                                                              //
//"Allow modechange" 0 is false(not allow) and 1 is true(allow).                                //
//   You can change mode with "start button" on controller if setting is on                     //
//                                                                                              //
//"Sendfocus"                                                                                   //
//   Method of focusing window before doing input                                               // 
//	0: None                                                                                 //
//	1: mouseclick upper left corner                                                         // 
//	2: SetForegroundWindow                                                                  //
//	3: SetActiveWindow                                                                      //  
//                                                                                              //
//"Keyresponsetime"										//
//	Milliseconds to simulate hold key. longer may feel more laggy on multi instance 	//
//	default: 50 ()       									// 
//                                                                                              // 
//"GetMouseOnKey"                                                                               //
//	instance will grab the mouse on key presses if set to 1. 0 = disabled                   //
//                                                                                              //
//"Ainputtype"  (A from A to F)                                                                 //
//      A button input setting:                                                                 //
//		0: move and click both cursors                                                  //
//              1: Only move emulated cursor                                                    //
//              2: Only click with real cursor                                                  //
//                                                                                              //
//                                                                                              //
//                                                                                              //
//                                                                                              //
//                                                                                              //
//                                                                                              //                                                             
//////////////////////////////////////////////////////////////////////////////////////////////////
[Hooks]
ClipCursorHook=1
GetKeystateHook=1
GetAsynckeystateHook=1
GetCursorposHook=1
SetCursorHook=1

[Settings]
Controllerid=0
AxisLeftsens=-7849
AxisRightsens=12000
AxisUpsens=0
AxisDownsens=-16049
Dot Speed=30
CA Dot Speed=50
Initial Mode=1
Allow modechange=1
Sendfocus=2 //obsolete
Keyresponsetime=300
GetMouseOnKey=1
Ainputtype=2
Binputtype=2
Xinputtype=1
Yinputtype=2
Cinputtype=2
Dinputtype=2
Einputtype=2
Finputtype=2
CursorPosHook=1
Righthanded=0
DrawFakeCursor=1
UseRealMouse=0
Scrollmapfix=0


  -     -
     *
   \___/
     | 
    \|/
   __|__
  /     \
 /       \
