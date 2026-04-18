vim.o.autochdir = false

vim.lsp.enable('clangd')

local tr = require("taskrunner")

vim.o.makeprg = 'cmake --build build'

vim.keymap.set('n', '<leader>rb', function()
    tr.run({ 'just', 'build', 'debug' }, {
        title      = "CMake Build Debug",
        id         = "cmake_build_debug",
    })
end)
vim.keymap.set('n', '<leader>rB', function()
    tr.run({ 'just', 'build', 'release' }, {
        title      = "CMake Build Release",
        id         = "cmake_build_release",
    })
end)
vim.keymap.set('n', '<leader>rc', function()
    tr.run({ 'just', 'clean' }, {
        title = "Clean",
        id = "clean"
    })
end)

vim.keymap.set('n', '<leader>rp', function() vim.cmd('botright 20sp') vim.cmd.terminal('./build/bin/PendulumSim') end)
vim.keymap.set('n', '<leader>fm', function() vim.cmd.edit('./src/main.cpp') end)

