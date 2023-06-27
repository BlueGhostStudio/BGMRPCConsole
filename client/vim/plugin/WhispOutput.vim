function! ShowShellOutput(command)
    if a:command !~ '^!'
        echo "第一个参数的第一个字符必须为 '!'"
        return
    endif

    let output = system('env INVIM=1 ' . substitute(a:command, '^!', '', ''))
    if len(trim(output)) == 0 && exists('g:whisp_keep_output') && g:whisp_keep_output
        return
    endif

    if !bufloaded('WhispShellOutput')
        if bufexists('WhispShellOutput')
            exec 'bw ' . 'WhispShellOutput'
        endif

        belowright new
        file WhispShellOutput
        " doautocmd User WhispBufNew
    else
        let winnr = bufwinnr('WhispShellOutput')
        if winnr != -1
            exec winnr . 'wincmd w'
        else
            belowright split WhispShellOutput
        endif
        setlocal modifiable
        exec '%delete _' | normal! gg

        if exists('b:whisp_keymap')
            for key in b:whisp_keymap
                execute 'unmap <buffer> ' . key
            endfor
        endif
        for key in keys(filter(copy(b:), 'v:key =~# "^whisp_"'))
            exec 'unlet b:' . key
        endfor

    endif

    setlocal nobuflisted
    setlocal buftype=nofile
    setlocal bufhidden=hide
    setlocal noswapfile

"    silent! exec '0r ' . a:command
    call setline(1, split(output, '\n'))
    setlocal nomodifiable

    let lines = getline(1, '$')
    let num_lines = len(lines)
    if num_lines == 1 && empty(lines[0])
        bd
    else
        normal! gg
        let window_height = (num_lines < 10) ? num_lines : 10

        exec 'resize ' . window_height

"        augroup DeleteBufferOnLeave
"            autocmd!
"            autocmd BufLeave,ModeChanged <buffer> bd
"        augroup END
        nnoremap <buffer> q :close <CR>
    endif
endfunction

command! -nargs=* Wo :call ShowShellOutput(<q-args>)

" function! ShowShellOutput(command)
"     call BaseShowShellOutput(a:command)
" endfunction
