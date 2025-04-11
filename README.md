FlowDriver - Multi-Protocol API Testing Tool
==========================================

FlowDriver is a versatile API testing tool that supports multiple protocols including REST, gRPC, WebSocket, and ZeroMQ. Built with Qt6 and modern C++, it provides a flexible and user-friendly interface for testing various API endpoints.

Features
--------
- Multi-protocol support:
  * REST API testing
  * gRPC with dynamic proto file loading
  * WebSocket client
  * ZeroMQ with multiple patterns (REQ-REP, PUB-SUB, PUSH-PULL, DEALER-ROUTER)
- Real-time message exchange
- Authentication support (Basic, Bearer, API Key)
- Request/Response monitoring
- JSON formatting
- SSL/TLS support
- Dynamic protocol switching

Prerequisites
------------
The following packages are required to build FlowDriver:

For Ubuntu/Debian:
```bash
sudo apt install protobuf-compiler-grpc \
                 libgrpc-dev \
                 libgrpc++-dev \
                 libprotobuf-dev \
                 protobuf-compiler \
                 libprotoc-dev \
                 libabsl-dev \
                 libboost-url-dev \
                 libboost-system-dev \
                 libboost-beast-dev \
                 libssl-dev \
                 libzmq3-dev \
                 rapidjson-dev \
                 qt6-base-dev \
                 qt6-declarative-dev \
                 qt6-websockets-dev \
                 build-essential \
                 cmake \
                 autoconf \
                 libtool \
                 pkg-config \
                 git \
                 nlohmann-json3-dev
```
Building
--------
1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/flowdriver.git
   cd flowdriver
   ```

2. Create build directory:
   ```bash
   mkdir build
   cd build
   ```

3. Configure and build:
   ```bash
   cmake ..
   make
   ```

Usage
-----
1. REST API Testing:
   - Select REST protocol
   - Choose HTTP method (GET, POST, PUT, DELETE, PATCH)
   - Enter URL
   - Add headers, body, query parameters
   - Click Send

2. gRPC Testing:
   - Select gRPC protocol
   - Load .proto file using Browse button
   - Select service and method
   - Configure endpoint
   - Enable/disable SSL
   - Enter request payload
   - Click Send

3. WebSocket Testing:
   - Select WebSocket protocol
   - Enter WebSocket URL (ws:// or wss://)
   - Click Connect
   - Send/receive messages in real-time

4. ZeroMQ Testing:
   - Select ZeroMQ protocol
   - Choose pattern (REQ-REP, PUB-SUB, etc.)
   - Select role (REQUESTER, REPLIER, etc.)
   - Enter endpoint
   - Click Connect
   - Exchange messages based on pattern

Project Structure
---------------
- include/         - Header files
- src/            - Source files
- qml/            - QML interface files
- proto/          - Protocol buffer definitions
- cmake/          - CMake configuration files

TESTING WEBSOCKET
For testing WebSocket connections, there are several publicly available WebSocket echo servers and testing services that you can use. These services allow you to connect, send messages, and receive responses, making them ideal for testing and debugging WebSocket clients. Here are some popular options:

---

### 1. **WebSocket Echo Servers**
These servers echo back any message you send, which is useful for testing basic WebSocket functionality.

- **wss://echo.websocket.org**
  A well-known WebSocket echo server. It responds with the same message you send.

- **wss://ws.postman-echo.com/socket**
  Provided by Postman, this WebSocket echo server is great for testing and debugging.

- **wss://echo.websocket.events**
  Another reliable WebSocket echo server.

---

### 2. **Public WebSocket APIs**
Some services provide WebSocket APIs for real-time data, such as cryptocurrency prices, stock prices, or weather updates.

- **wss://stream.binance.com:9443/ws/btcusdt@trade**
  Binance WebSocket API for real-time cryptocurrency trading data (e.g., BTC/USDT trades).

- **wss://ws.coincap.io/prices?assets=bitcoin,ethereum**
  CoinCap WebSocket API for real-time cryptocurrency prices.

- **wss://ws-feed.pro.coinbase.com**
  Coinbase Pro WebSocket API for real-time market data.

- **wss://api-pub.bitfinex.com/ws/2**
  Bitfinex WebSocket API for cryptocurrency trading data.

---

### 3. **Testing and Debugging Tools**
These tools provide WebSocket endpoints for testing and debugging.

- **websocat**
  A command-line WebSocket tool that can connect to WebSocket servers or act as a server.
  GitHub: [https://github.com/vi/websocat](https://github.com/vi/websocat)

- **Postman**
  Postman supports WebSocket testing. You can connect to WebSocket endpoints and send/receive messages.
  Website: [https://www.postman.com](https://www.postman.com)

- **WebSocket King**
  A browser-based WebSocket client for testing WebSocket connections.
  Website: [https://websocketking.com](https://websocketking.com)

---

### 4. **Custom WebSocket Servers**
If you need more control, you can set up your own WebSocket server for testing. Libraries like **Node.js (ws)**, **Python (websockets)**, or **Go (gorilla/websocket)** make it easy to create a WebSocket server.

---

### Summary
For quick testing, start with these public WebSocket servers:
- **wss://echo.websocket.org**
- **wss://ws.postman-echo.com/socket**
- **wss://echo.websocket.events**

For real-time data, try:
- **wss://stream.binance.com:9443/ws/btcusdt@trade**
- **wss://ws.coincap.io/prices?assets=bitcoin,ethereum**

For debugging, use tools like **Postman** or **WebSocket King**. If you need more control, consider setting up your own WebSocket server.

TESTING REST
For testing REST APIs, there are several publicly available services that provide endpoints for various HTTP methods (GET, POST, PUT, DELETE, etc.). These services are great for testing and debugging REST clients. Here are some popular options:

---

### 1. **httpbin.org**
   - A simple HTTP request and response service.
   - Provides endpoints for testing various HTTP methods, headers, authentication, and more.
   - Example endpoints:
     - `https://httpbin.org/get` - Returns GET data.
     - `https://httpbin.org/post` - Returns POST data.
     - `https://httpbin.org/put` - Returns PUT data.
     - `https://httpbin.org/delete` - Returns DELETE data.
     - `https://httpbin.org/status/{code}` - Returns the specified HTTP status code.
     - `https://httpbin.org/headers` - Returns request headers.
   - Website: [httpbin.org](https://httpbin.org)

---

### 2. **JSONPlaceholder**
   - A free online REST API for testing and prototyping.
   - Simulates a simple blog with posts, comments, users, and more.
   - Example endpoints:
     - `https://jsonplaceholder.typicode.com/posts` - Get all posts.
     - `https://jsonplaceholder.typicode.com/posts/1` - Get a specific post.
     - `https://jsonplaceholder.typicode.com/comments?postId=1` - Get comments for a post.
     - `https://jsonplaceholder.typicode.com/users` - Get all users.
   - Website: [JSONPlaceholder](https://jsonplaceholder.typicode.com)

---

### 3. **ReqRes**
   - A hosted REST API ready to respond to your requests.
   - Provides endpoints for testing CRUD operations.
   - Example endpoints:
     - `https://reqres.in/api/users` - Get a list of users.
     - `https://reqres.in/api/users/2` - Get a specific user.
     - `https://reqres.in/api/users` - Create a new user (POST).
     - `https://reqres.in/api/users/2` - Update a user (PUT/PATCH).
     - `https://reqres.in/api/users/2` - Delete a user (DELETE).
   - Website: [ReqRes](https://reqres.in)

---

### 4. **MockAPI**
   - A tool to create mock REST APIs for testing and prototyping.
   - You can create custom endpoints and define responses.
   - Example endpoints:
     - `https://mockapi.io/projects/{projectId}/endpoints`
   - Website: [MockAPI](https://mockapi.io)

---

### 5. **Postman Echo**
   - Provided by Postman, this service echoes back requests for testing.
   - Example endpoints:
     - `https://postman-echo.com/get` - Echo GET request.
     - `https://postman-echo.com/post` - Echo POST request.
     - `https://postman-echo.com/put` - Echo PUT request.
     - `https://postman-echo.com/delete` - Echo DELETE request.
   - Website: [Postman Echo](https://postman-echo.com)

---

### 6. **Public APIs**
   - Many public APIs can be used for testing REST clients. Examples include:
     - **OpenWeatherMap** (`https://api.openweathermap.org/data/2.5/weather`) - Weather data.
     - **GitHub API** (`https://api.github.com/users/{username}`) - GitHub user data.
     - **PokéAPI** (`https://pokeapi.co/api/v2/pokemon/{id}`) - Pokémon data.
     - **NASA API** (`https://api.nasa.gov/planetary/apod`) - Astronomy Picture of the Day.

---

### 7. **Local Testing Tools**
   - If you need more control, you can set up a local REST server for testing:
     - **JSON Server** (Node.js) - Quickly create a mock REST API from a JSON file.
       - GitHub: [JSON Server](https://github.com/typicode/json-server)
     - **Postman Mock Server** - Create a mock server using Postman.
       - Website: [Postman](https://www.postman.com)

---

### Summary
For quick testing, start with these services:
- **httpbin.org** - General HTTP testing.
- **JSONPlaceholder** - Simulates a blog with posts, comments, and users.
- **ReqRes** - CRUD operations for users.

For more advanced testing, use:
- **MockAPI** - Create custom mock APIs.
- **Postman Echo** - Echo requests for debugging.

For real-world data, explore public APIs like **OpenWeatherMap**, **GitHub API**, or **PokéAPI**. If you need full control, set up a local server using tools like **JSON Server** or **Postman Mock Server**.

TESTING GRPC
For testing gRPC, there are several publicly available services and tools that provide gRPC endpoints for testing and debugging. These services allow you to interact with gRPC APIs, send requests, and receive responses. Here are some popular options:

---

### 1. **grpcb.in**
   - A public gRPC testing service provided by the gRPC community.
   - Provides endpoints for testing gRPC methods, including unary, server streaming, client streaming, and bidirectional streaming.
   - Example endpoints:
     - `grpcb.in:9000` - Echo service (echoes back requests).
     - `grpcb.in:9001` - Health check service.
   - Website: [grpcb.in](https://grpcb.in)

---

### 2. **Evans**
   - A CLI tool for interacting with gRPC servers.
   - You can use Evans to test gRPC endpoints by connecting to a server and invoking methods.
   - Example:
     ```bash
     evans -r -p 50051 grpcb.in
     ```
   - GitHub: [Evans](https://github.com/ktr0731/evans)

---

### 3. **BloomRPC**
   - A GUI client for testing gRPC services.
   - Allows you to load `.proto` files, connect to gRPC servers, and invoke methods.
   - Website: [BloomRPC](https://github.com/uw-labs/bloomrpc)

---

### 4. **Postman (gRPC Support)**
   - Postman now supports gRPC, allowing you to test gRPC endpoints directly.
   - You can import `.proto` files, connect to gRPC servers, and send requests.
   - Website: [Postman](https://www.postman.com)

---

### 5. **gRPCurl**
   - A command-line tool for interacting with gRPC servers.
   - Similar to `curl` but for gRPC.
   - Example:
     ```bash
     grpcurl -plaintext grpcb.in:9000 list
     grpcurl -plaintext grpcb.in:9000 grpcbin.GRPCBin/Echo
     ```
   - GitHub: [gRPCurl](https://github.com/fullstorydev/grpcurl)

---

### 6. **Local Testing Tools**
   - If you need more control, you can set up a local gRPC server for testing:
     - **grpc-go** - A Go implementation of gRPC.
       - GitHub: [grpc-go](https://github.com/grpc/grpc-go)
     - **grpc-java** - A Java implementation of gRPC.
       - GitHub: [grpc-java](https://github.com/grpc/grpc-java)
     - **grpc-node** - A Node.js implementation of gRPC.
       - GitHub: [grpc-node](https://github.com/grpc/grpc-node)

---

### 7. **Public gRPC APIs**
   - Some public APIs provide gRPC endpoints for testing:
     - **Google APIs** - Many Google services (e.g., Google Cloud) provide gRPC endpoints.
     - **etcd** - A distributed key-value store with gRPC support.
       - Website: [etcd](https://etcd.io)

---

### Summary
For quick testing, start with these services:
- **grpcb.in** - Public gRPC testing service.
- **Evans** - CLI tool for interacting with gRPC servers.
- **BloomRPC** - GUI client for testing gRPC services.
If you need full control, set up a local gRPC server using tools like **grpc-go**, **grpc-java**, or **grpc-node**. For real-world data, explore public gRPC APIs like **Google APIs** or **etcd**.


