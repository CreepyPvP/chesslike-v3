vim.keymap.set("n", "<", "<cmd>tabnew term://cd build && make && ./strategy<CR>");
vim.keymap.set("n", ">", "<cmd>tabnew term://cd build && make && gdb ./strategy<CR>");
