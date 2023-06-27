function! ClearUndo()
    let old_undolevels = &undolevels
    set undolevels=-1
    exec "normal a \<BS>\<ESC>"
    let &undolevels = old_undolevels
    unlet old_undolevels
endfunction

function! LoadContent(content)
    let b:loadstatus = 1

    call setline(1, a:content)
    call ClearUndo()
    let b:modified = 0
    let b:savedUndoSeq = undotree().seq_cur

    if has('nvim')
        exec "normal a \<BS>\<ESC>"
    endif

"    exec "set syntax=" . b:syn
endfunction

" if !exists('g:CMS#Statusline')
"     let g:CMS#Statusline = 0
" endif

if !exists('g:CMS#Statusline')
    highlight StatusLine cterm=NONE ctermfg=Black ctermbg=LightGrey
    highlight StatusLineNC cterm=NONE ctermfg=Black ctermbg=Grey
    highlight StatusHighlight cterm=NONE ctermfg=Black ctermbg=LightCyan
    highlight StatusHighlightNC cterm=NONE ctermfg=Black ctermbg=Cyan
    highlight StatusLogo cterm=NONE ctermfg=Black ctermbg=Cyan cterm=bold
    highlight StatusLogoNC cterm=NONE ctermfg=White ctermbg=DarkCyan

    function! CMSStatusLine()
        let bufID = winbufnr(g:statusline_winid)

        if win_getid() == g:statusline_winid
            let logoHL = 'StatusLogo'
            let __highlight__ = 'StatusHighlight'
        else
            let logoHL = 'StatusLogoNC'
            let __highlight__ = 'StatusHighlightNC'
        endif

        let l:modified = getbufvar(bufID, 'modified') ? ' [+]' : ''
        "
        let which = getbufvar(bufID, 'which')
        if which == 0
            let which = ''
        elseif which == 1
            let which = ' » SUMMARY «'
        elseif which == 2
            let which = ' » EXTDATA «'
        endif

        let l:mode = mode()
        if l:mode == 'n'
            let l:mode = 'NORMAL'
        elseif l:mode == 'v'
            let l:mode = 'VISUAL'
        elseif l:mode == 'i'
            let l:mode = 'INSERT'
        elseif l:mode == 'c'
            let l:mode = 'COMMAND'
        endif

        let l:path = getbufvar(bufID, 'path')
        return '%#' . logoHL .
                    \ '# BGMRPC-CMS %*'
                    \ . '%#' . __highlight__ . '# '
                    \ . l:path
                    \ . which
                    \ . l:modified
                    \ . ' %*%=%#' . __highlight__ . '# '
                    \ . l:mode . ' ' . '%l,%c %P %*'
    endfunction
endif

if exists('g:CMS#Statusline') && g:CMS#Statusline ==# 'lightline'
    let g:lightline = {
                \   'colorscheme': 'one',
                \   'component_function': {
                \       'filename': 'CMS_lightline_filename',
                \       'modified': 'CMS_lightline_modified',
                \   },
                \   'component_visible_condition': {
                \       'modified': '&modified||b:modified'
                \   }
                \}

    function! CMS_lightline_filename()
        if exists('b:cmsbuftype') && b:cmsbuftype ==# 'cms'
            let fileName = b:path
            if b:which == 1
                let fileName .= ' ‹SUMMARY›'
            elseif b:which == 2
                let fileName .= ' ‹EXTDATA›'
            endif
            return fileName
        else
            return expand('%:t')
        endif
    endfunction

    function! CMS_lightline_modified()
        if (exists('b:cmsbuftype') && b:cmsbuftype ==# 'cms' && b:modified)
                    \ || &modified
            return '[+]'
        else
            return ''
        endif
    endfunction 
endif

if exists('g:CMS#Statusline') && g:CMS#Statusline ==# 'airline'
    let g:airline_section_c = '%{CMS_airline_filename()}'

    function! CMS_airline_filename()
        let fileName = ''
        if exists('b:cmsbuftype') && b:cmsbuftype ==# 'cms'
            let fileName = b:path
            if b:which == 1
                let fileName .= ' ‹SUMMARY›'
            elseif b:which == 2
                let fileName .= ' ‹EXTDATA›'
            endif
        else
            let fileName = expand('%:t')
        endif

        if (exists('b:cmsbuftype') && b:cmsbuftype ==# 'cms' && b:modified)
                    \ || &modified
            let fileName .= '[+]'
        endif

        return fileName
    endfunction
endif

command! -nargs=1 Wec :call WhispCmsEdit(<args>, 0)
command! -nargs=1 Wes :call WhispCmsEdit(<args>, 1)
command! -nargs=1 Wee :call WhispCmsEdit(<args>, 2)

function! WhispCmsEdit(id, which)
    let id = shellescape(a:id)
    let workspace = substitute(system('whisp use 3>&1 1>/dev/null'), '\n', '', '')
    let json = system('whisp call main node --workspace=' . workspace . ' --app=cms -- ' . id)
    let json = json_decode(json)

    if json[0].ok
        let node = json[0].node
        let nodeName = substitute(node.name, '\n', '', '')
        let fileName = node.id . '_' . nodeName
        if a:which == 1
            let fileName = fileName . '_summary'
        elseif a:which == 2
            let fileName = fileName . '_extData'
        endif

        if bufexists(fileName)
            if bufloaded(fileName)
                exec 'buffer ' . fileName
                return
            else
                exec 'bw ' . fileName
            endif
        endif

        let editable = 1
        let bufSyn = a:which == 0 ? 'none' : 'md'

        if a:which == 0
            let contentType = node.contentType
            if contentType =~ '\vhtml|fra|cmp|pkg|elm'
                let bufSyn = 'html'
            elseif contentType == 'js'
                let bufSyn = 'javascript'
            elseif contentType == 'json'
                let bufSyn = 'json'
            elseif contentType == 'md'
                let bufSyn = 'markdown'
            elseif contentType =~ '\vstyle|css|less'
                let bufSyn = 'less'
            elseif contentType =~ '\vjpg|jpeg|png|gif'
                echohl ErrorMsg | echo "Can't open image" | echohl None
                let editable = 0
            endif
        endif

        if editable
            if bufname('%') == 'WhispShellOutput'
                wincmd w
            endif

            enew
            setlocal buftype=nofile
            setlocal bufhidden=hide
            setlocal modifiable
            setlocal nomodified
            exec 'setlocal filetype=' . bufSyn

            let b:cmsbuftype = 'cms'
            let b:id = node.id
            let b:syn = bufSyn
            let b:workspace = workspace
            let b:which = a:which
            let b:path = ''

            cnoreabbrev <buffer> bd WBD

"            let bufid = bufnr()
"            call autocmd_add([#{ event: 'TextChanged', bufnr: bufid, cmd: 'call OnTextChanged()', group: 'WhispBuf' }])
"            call autocmd_add([#{ event: 'InsertCharPre', bufnr: bufid, cmd: 'autocmd TextChangedI <buffer=abuf> call OnTextChanged()', group: 'WhispBuf'}])
"            call autocmd_add([#{ event: 'InsertLeave', bufnr: bufid, cmd: 'autocmd! TextChangedI <buffer=abuf>', group: 'WhispBuf' }])

            " let w:airline_disabled = exists('b:id')
            let json = system('whisp call main nodePath --workspace=' . workspace . ' --app=cms -- ' . a:id)
            let json = json_decode(json)

            let b:path = json[0].str

            silent! exec "file " . fileName

            if a:which == 0
                call LoadContent(split(node.content, '\n'))
            elseif a:which == 1
                call LoadContent(split(node.summary, '\n'))
            elseif a:which == 2
                call LoadContent(split(node.extData, '\n'))
            endif
            
            if !exists('g:CMS#Statusline')
                setlocal statusline=%!CMSStatusLine()
            endif
        endif
    endif
endfunction

function! RenameCmsBuffer(bufnr, name)
    let id = getbufvar(a:bufnr, 'id')
    let workspace = getbufvar(a:bufnr, 'workspace')
    let curBufnr = bufnr('%')
    exec 'buffer ' . a:bufnr
    exec 'file ' . id . '_' . a:name
    exec 'buffer ' . curBufnr
    let json = system('whisp call main nodePath --workspace=' . workspace . ' --app=cms -- ' . id)
    call setbufvar(a:bufnr, 'path', json_decode(json)[0].str)
endfunction

function! UpdateCmsContentBuffer(bufnr, content)
    " 判断目标缓冲区的内容是否与给定文本相等
    let localContent = join(getbufline(a:bufnr, 1, '$'), "\n")
    if localContent == a:content
        return
    endif

    let diff_cmd = printf('diff <(echo %s) <(echo %s)',
                \ shellescape(localContent),
                \ shellescape(a:content))
    let diff_output = system(diff_cmd)

    " 使用 echom 命令将 diff_output 输出到 Vim 的消息缓冲区
    echo diff_output

    echohl Question
    let user_input = input("The content has been modified remotely. " . 
                \ "Do you want to synchronize? Type 'yes' to confirm: ")
    echohl none

    if user_input ==? 'yes'
        let curbufnr = bufnr('%')
        exec 'buffer ' . a:bufnr
        call LoadContent(split(a:content, "\n"))
        exec 'buffer ' . curbufnr
    else
        echo "Synchronization cancelled."
    endif

"    " 切换到目标缓冲区窗口
"    execute 'buffer ' . a:bufnr
"    diffthis
"
"    " 在当前窗口右侧打开一个新窗口并加载 content 内容
"    vnew
"    wincmd R
"    setlocal buftype=nofile bufhidden=hide noswapfile
"    call setline(1, split(a:content, '\n'))
"    exec 'file ' . bufname(bufnr) . '(remote)'
"    let b:cmsbuftype = 'cms,remote'
"
"    " 对当前窗口和目标缓冲区进行 diff
"    diffthis
endfunction

command! Ww :call WhispCmsUpdate()

function! WhispCmsUpdate()
    if exists('b:id') && exists('b:workspace') && exists('b:modified') && b:modified
        let node = {}
        let content = join(getline(1, '$'), "\n")
        if b:which == 0
            let node["content"] = content
        elseif b:which == 1
            let node["summary"] = content
        elseif b:which == 2
            let node["extData"] = content
        endif

        let retJson = system('whisp call main updateNode --workspace='
                    \ . b:workspace . ' --app=cms -- '
                    \ . b:id . ' ' . shellescape(json_encode(node)))
        let retJson = json_decode(retJson)[0]

        if retJson.ok
            let b:modified = 0
        else
            echohl ErrorMsg | echo retJson.error | echohl None
        endif

"        let contentJson = '{"content": ' . json_encode(join(getline(1, '$'), "\n")) . '}'
"        let retJson = system('whisp call main updateNode --workspace='
"                    \ . b:workspace . ' --app=cms -- '
"                    \ . b:id . ' ' . shellescape(contentJson))
"        let retJson = json_decode(retJson)[0]
"
"        if retJson.ok
"            let b:modified = 0
"        else
"            echohl ErrorMsg | echo retJson.error | echohl None
"        endif
    endif
endfunction

command! -bang WBD :call WhispBufDel(<bang>0)

function! WhispBufDel(force)
    if exists('b:id') && exists('b:modified') && b:modified && !a:force
        echohl ErrorMsg | echo "This buffer has been modified and cannot be deleted. Execute ':bd!' to force deletion." | echohl None
    else
        exec 'bw'
    endif
endfunction

function! OnTextChanged()
    if exists('b:id')
        let seq_cur = undotree().seq_cur
        if exists('b:loadstatus') && b:loadstatus == 1
            let b:loadstatus = 0
        elseif seq_cur == b:savedUndoSeq || seq_cur == 0
            let b:modified = 0
        else
            let b:modified = 1
        endif
    endif
endfunction

augroup WhispBuf
    autocmd!
    autocmd TextChanged * call OnTextChanged()
    autocmd InsertCharPre * autocmd TextChangedI <buffer=abuf> call OnTextChanged()
    autocmd InsertLeave * autocmd! TextChangedI <buffer=abuf> call OnTextChanged()
augroup END
" autocmd BufWinEnter,BufEnter * call CMSStatusLine()
