const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');

const backendDir = path.join(__dirname, '..', 'packages', 'backend');
const exe = path.join(backendDir, 'spreadsheet_server.exe');

if (!fs.existsSync(exe)) {
  console.log('Building C++ backend...');
  const { execSync } = require('child_process');
  execSync(
    'g++ -std=c++14 -O2 -Wall -Iinclude -o spreadsheet_server.exe src/main.cpp src/DependencyGraph.cpp src/TopoSort.cpp src/FormulaParser.cpp src/SpreadsheetEngine.cpp src/WebSocketServer.cpp -lws2_32',
    { cwd: backendDir, stdio: 'inherit' }
  );
}

const child = spawn(exe, ['3001'], { cwd: backendDir, stdio: 'inherit' });
child.on('exit', (code) => process.exit(code || 0));
process.on('SIGINT', () => child.kill());
process.on('SIGTERM', () => child.kill());
