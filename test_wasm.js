const fs = require('fs');

async function runWasm() {
    try {
        // 读取WASM文件
        const wasmBuffer = fs.readFileSync('test.wasm');
        
        // 编译WASM模块
        const wasmModule = await WebAssembly.compile(wasmBuffer);
        
        // 实例化WASM模块
        const wasmInstance = await WebAssembly.instantiate(wasmModule);
        
        // 调用main函数
        const result = wasmInstance.exports.main();
        
        console.log('WASM执行结果验证');
        console.log('================');
        console.log(`调用 main() 函数的结果: ${result}`);
        console.log('');
        console.log('预期计算过程:');
        console.log('1. x = 42');
        console.log('2. y = x + 10 = 42 + 10 = 52');
        console.log('3. return y * 2 = 52 * 2 = 104');
        console.log('');
        
        if (result === 104) {
            console.log('✅ 测试通过！WASM模块正确执行了C代码逻辑');
        } else {
            console.log(`❌ 测试失败！期望结果是104，实际结果是${result}`);
        }
        
    } catch (error) {
        console.error('❌ WASM执行失败:', error.message);
    }
}

runWasm();