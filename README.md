# SO2_PipeDream


![Screenshot_1](https://github.com/pmspedrosa/SO2_PipeDream/assets/76016818/f8947554-7d8d-4c28-a229-a5f800a47f60)
![Screenshot_2](https://github.com/pmspedrosa/SO2_PipeDream/assets/76016818/1103a6ad-8c87-43e8-b3bc-4e4e85cb4640)


1. **Elements of the Game and General Logic**:
   - The game consists of a server, a monitor, and one or two clients.
     - **Server**: controls the game and manages the game map.
     - **Monitor**: displays the game state in real-time and can modify the game.
     - **Client**: is used by players to interact with the server and play the game.

2. **Usage and Detailed Functionality**:
   - **Server**:
     - should be launched first and can only have one instance.
     - manages the game map, water flow, and player actions.
   - **Client**:
     - can be launched for individual or competitive gameplay.
     - interacts with the server and not directly with the monitor.
   - **Monitor**:
     - displays real-time game maps and can modify game behavior.

3. **Communication Between Applications**:
   - The server communicates with clients via named pipes.
   - The server communicates with the monitor using shared memory.
   - Asynchronous and synchronization mechanisms.
  

**How to use:**

1. Run server
2. Run monitor
3. Run client (this can be one or two)
4. Go to the Menu
   -  ![Screenshot_4](https://github.com/pmspedrosa/SO2_PipeDream/assets/76016818/af03b5a8-ef11-48a5-b726-80a626659539)
5. Write your name
6. Start the game(Single/Multiplayer)
7. Press the tiles and discover the game habilities :)


