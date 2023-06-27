command! -bang -nargs=* Whisp :call WhispCmsCall(<bang>0, <q-args>)

function! WhispCmsCall(ShellWin, command)
    if a:ShellWin
        exec '!whisp ' . a:command
    else
        let tmpFile = tempname()
        call ShowShellOutput('!whisp ' . a:command . ' 4>' . tmpFile)

        let vimData = readfile(tmpFile)
        if !empty(vimData)
            for cmd in vimData
                exec cmd
            endfor
        endif

        call delete(tmpFile)
    endif
endfunction

function! WhispCmsConfirmRemove(path)
    echohl Question
    let user_input = input("Confirm deletion of " . a:path . "? " .
                \ "Type 'yes' to confirm: ")
    echohl none

    if user_input ==? 'yes'
        exec 'Whisp @remove ' . a:path . ' -f'
    endif
endfunction

function! WhispCMSList_remove()
let index = line('.') - 3
    if index >= 0 && index < len(b:whisp_cmsList)
        let node = b:whisp_cmsList[index]
        let curWinNr = winnr()
        let g:whisp_keep_output = 1
        call WhispCmsConfirmRemove(node.name)
        let g:whisp_keep_output = 0
        exec curWinNr . 'wincmd w'
    endif
endfunction

function! KeyMap(key, mapping)
    if !exists('b:whisp_keymap')
        let b:whisp_keymap = []
    endif

    let b:whisp_keymap += [a:key]
    execute 'nnoremap <buffer> <silent> ' . a:key . ' ' . a:mapping
endfunction

function! WhispCMSList_itemEnter()
    let index = line('.') - 3
    if index >= 0 && index < len(b:whisp_cmsList)
        let node = b:whisp_cmsList[index]
        if node.type == 'D'
            silent! exec 'Whisp @cd ' . node.name
            silent! Whisp @list
        else
            call WhispCMSList_edit(0)
        endif
    endif
endfunction

function! WhispCMSList_edit(which)
    let index = line('.') - 3
    if index >= 0 && index < len(b:whisp_cmsList)
        let node = b:whisp_cmsList[index]
        let curWinNr = winnr()
        let g:whisp_keep_output = 1
        if a:which == 0
            silent! exec 'Whisp @edit ' . node.name
        elseif a:which == 1
            silent! exec 'Whisp @edit ' . node.name . ' -s -C'
        elseif a:which == 2
            silent! exec 'Whisp @edit ' . node.name . ' -e -C'
        endif
        let g:whisp_keep_output = 0
        exec curWinNr . 'wincmd w'
    endif
endfunction

function! WhispCMSList_update()
    let index = line('.') - 3
    if index >= 0 && index < len(b:whisp_cmsList)
        let node = b:whisp_cmsList[index]
        silent! exec '!whisp @update ' . node.name
    endif
endfunction

function! WhispCMSList(list, id)
    let l:wh = winheight(0) + 7
    exec 'resize ' . l:wh
    setlocal modifiable
    call append('$', [
                \ "-------------------------------------------------------",
                \ "Press 'n' to create new node",
                \ "Press 'r' to remove node",
                \ "Press 's' to edit the description text",
                \ "press 'e' to edit the extended data",
                \ "Press 'u' update node properties",
                \ "press 'Enter' to edit the content/change the directory."])
    setlocal nomodifiable
    let b:whisp_cmsList = json_decode(a:list)
    let b:whisp_cmsListID = a:id
    let b:cmsbuftype = 'cms,list'
    call KeyMap('n', ':Whisp! @new<CR>')
    call KeyMap('r', ':call WhispCMSList_remove()<CR>')
    call KeyMap('s', ':call WhispCMSList_edit(1)<CR>')
    call KeyMap('e', ':call WhispCMSList_edit(2)<CR>')
    call KeyMap('u', ':call WhispCMSList_update()<CR>')
    call KeyMap('<CR>', ':call WhispCMSList_itemEnter()<CR>')
endfunction

"let g:sigLog = {}

function! EachBuf(cmsbuftype, cb, args) abort
    for buf in getbufinfo()
        if has_key(buf.variables, 'cmsbuftype')
                    \ && buf.variables.cmsbuftype =~ a:cmsbuftype
            call a:cb(buf, a:args)
        endif
    endfor
endfunction

let g:testLog = []

function! ListChanged(sigData)
    let curWinNr = winnr()
    let curBufNr = bufnr()

    let bufWinnr = bufwinnr('WhispShellOutput')
    let bufnr = bufnr('WhispShellOutput')
    if bufWinnr != -1
        exec bufWinnr . 'wincmd w'
    elseif buffer_exists(bufnr) && bufloaded(bufnr)
        exec 'buffer ' . bufnr
    else
        return
    endif

    let refresh = 0
    if a:sigData.signal == 'nodeCreated'
        if a:sigData.args[0].node.pid == b:whisp_cmsListID
            let refresh = 1
        endif
    elseif a:sigData.signal == 'nodeRemoved'
        " todo
        let removedNodeID = a:sigData.args[0]
        for listItem in b:whisp_cmsList
            if listItem.id == removedNodeID
                let refresh = 1
                break
            endif
        endfor
    endif

    if refresh
        setlocal modifiable
        call deletebufline(bufnr, line('$') - 5, '$')

        if exists('b:whisp_keymap')
            for key in b:whisp_keymap
                execute 'unmap <buffer> ' . key
            endfor
        endif
        unlet b:whisp_keymap

        call append('$', "The content list has been updated. Press 'r' to refresh.")
        setlocal nomodifiable

        call KeyMap('r', ':Whisp @list<CR>')
    endif

    exec curWinNr . 'wincmd w'
    exec 'buffer ' . curBufNr
endfunction

function! UpdateNode(buf, sigData)
    if a:buf.variables.workspace == g:workspace
                \ && a:sigData.args[0] == a:buf.variables.id
        if a:sigData.signal == 'nodeRenamed'
            call RenameCmsBuffer(a:buf.bufnr, a:sigData.args[1])
        elseif a:sigData.signal == 'contentUpdated'
                    \ && a:buf.variables.which == 0
            call UpdateCmsContentBuffer(a:buf.bufnr, a:sigData.args[1])
        elseif a:sigData.signal == 'summaryUpdated'
                    \ && a:buf.variables.which == 1
            call UpdateCmsContentBuffer(a:buf.bufnr, a:sigData.args[1])
        elseif a:sigData.signal == 'extDataUpdated'
                    \ && a:buf.variables.which == 2
            call UpdateCmsContentBuffer(a:buf.bufnr, a:sigData.args[1])
        endif
    endif
endfunction

function! WatchCMSSignal(channel_id, data)
    sleep 500m
    let sigData = json_decode(a:data)
    if sigData.signal =~ '\vnodeRenamed|contentUpdated|summaryUpdated|extDataUpdated'
        call EachBuf('^cms$', function('UpdateNode'), sigData)
    elseif sigData.signal =~ '\vnodeRemoved|nodeCreated'
        call ListChanged(sigData)
        " let g:testLog += [a:data]
        " cexpr g:testLog
    endif
endfunction

function! StartWatchCMSSignalJob(workspace)
    if exists('g:watch_cms_signal_jobID')
        call job_stop(g:watch_cms_signal_jobID)
    endif
    let g:workspace = a:workspace
    let g:watch_cms_signal_jobID = job_start('whisp watch ' .
                \ '--signal --obj=main --app=cms ' .
                \ '--workspace=' . a:workspace, #{
                \ out_cb: function('WatchCMSSignal'),
                \ out_mode: 'nl',
                \ })
endfunction

function! WatchWhispWSPChanged(channel_id, data)
    call StartWatchCMSSignalJob(a:data)
endfunction

call job_start('whisp watch --WSPChanged', #{
            \ out_cb: function('WatchWhispWSPChanged'),
            \ out_mode: 'nl',
            \ })


let workspace = system('whisp use 3>&1 1>/dev/null')
if !empty(workspace)
    call StartWatchCMSSignalJob(workspace)
endif

