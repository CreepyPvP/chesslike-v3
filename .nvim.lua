-- vim.keymap.set("n", 
--         "<", 
--         "<cmd>tabnew term://make && cd build && make && ./strategy<CR>");
-- vim.keymap.set("n",
--         ">", 
--         "<cmd>tabnew term://make && cd build && make && gdb ./strategy<CR>");
vim.keymap.set("n", 
    "<",
    "<cmd>tabnew term://make && cd build && msbuild strategy.sln && cd Debug && strategy.exe<CR>");
