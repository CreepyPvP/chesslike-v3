-- vim.keymap.set("n", 
--         "<", 
--         "<cmd>tabnew term://make && cd build && make && ./strategy<CR>");
-- vim.keymap.set("n",
--         ">", 
--         "<cmd>tabnew term://make && cd build && make && gdb ./strategy<CR>");
vim.keymap.set("n", "<", "<cmd>tabnew term://cd build && msbuild strategy.sln<CR>");
