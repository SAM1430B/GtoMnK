
Description:
A dll/plugin that Emulates mouse input with an Xinput controller. Coordinates are decided by presaved .BMP 24 bit format files. 
When a button is pressed on the controller, the game window content will be copied to memory. 
Then the presaved bmp will be compared to each pixel position until it fits, similar to a puzzle.
If the presaved bmp do fit in a location, a mouse click command will be sent to the coordinates found.

Setup guide:
1: Start the game you want to add this dll.
1: Make screenshots of the game window with "print screen" button.
2: Paste image in paint, save small images as Button(A,B,X,Y) + 0. example: B0.bmp in game directory(same directory as .exe and .dll).
3: If a button need to send input on more than 1 bmp then name next to B1.bmp progressively up to 10. Let me know if more are needed.
4: Use a dll loader to load the dll with a game/app. 
5: Enjoy.

Comments:
A scaling function is possible but not added yet. Have an idea for it, so i might add it later.
if the upper left corner of the window is presaved, then the BMP could be stretched and compared, stretched and compared ...... until it fits.
if it fits after stretching it will have found the stretching values needed for the other presaved BMPs?

Another idea for scaling is to compare window width and length. then a cfg file would be needed. Unsure if that would work also


  -     -
     *
   \___/
     | 
    \|/
   __|__
  /     \
 /       \
