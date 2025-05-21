// 代码编辑器实现
class CodeEditor 
{
    constructor(textareaId) 
    {
        this.textarea = document.getElementById(textareaId);
        this.editorContainer = document.createElement('div');
        this.lineNumbers = document.createElement('div');
        this.highlight = document.createElement('div');
        this.curLanguage = document.getElementById("language-selector").querySelector("option[selected]").value;
        this.codeMap = {};
        this.bracketMap = {
            '(': { pair: '()' },
            '[': { pair: '[]' },
            '{': { pair: '{}' },
            "'": { pair: "''" },
            '"': { pair: '""' }
        };
        
        this.setupEditor();
        this.setupEventListeners();
    }
    
    setupEditor() 
    {
        // 创建编辑器容器
        this.editorContainer.className = 'editor-container';
        this.textarea.parentNode.insertBefore(this.editorContainer, this.textarea);
        this.editorContainer.appendChild(this.textarea);
        
        // 创建行号
        this.lineNumbers.className = 'editor-line-numbers';
        this.editorContainer.appendChild(this.lineNumbers);
        
        // 创建代码高亮层
        this.highlight.className = 'editor-highlight';
        this.editorContainer.appendChild(this.highlight);
        
        // 设置初始样式
        this.update();
        this.setupCodeStorge();
    }
    
    setupEventListeners() 
    {
        // 滚动同步
        this.editorContainer.addEventListener('scroll', () => {
            this.syncScrollPositions();
        });

        // 文本变化事件
        this.textarea.addEventListener('input', () => {
            this.update();
        });
        
        // 键盘事件处理
        this.textarea.addEventListener('keydown', (e) => {
            const config = this.bracketMap[e.key];
            if (e.key === 'Tab') 
            {
                e.preventDefault();
                this.insertText('    ');
            }
            else if (e.key === 'Enter') 
            {
                const currentLine = this.getCurrentLine();
                const indent = currentLine.match(/^\s*/)[0];
                setTimeout(() => { this.insertText(indent); }, 0);
            }
            else if (config !== undefined)
            {
                e.preventDefault();
                const start = this.textarea.selectionStart;
                const end = this.textarea.selectionEnd;
                this.textarea.value = this.textarea.value.slice(0, start) + config.pair[0] + config.pair[1] + this.textarea.value.slice(end);
                this.textarea.selectionStart = this.textarea.selectionEnd = start + 1;
            }
            requestAnimationFrame(() => {
                this.highlight.scrollLeft = this.textarea.scrollLeft;
            });
        });
    }

    syncScrollPositions() 
    {
        const container = this.editorContainer;
        const scrollTop = container.scrollTop;
        const scrollLeft = container.scrollLeft;

        // 垂直同步所有元素
        this.textarea.scrollTop = scrollTop;
        this.lineNumbers.scrollTop = scrollTop;
        this.highlight.scrollTop = scrollTop;

        // 水平同步代码基底和高亮
        this.textarea.scrollLeft = scrollLeft;
        this.highlight.scrollLeft = scrollLeft;

        // 行号区域保持水平位置0
        this.lineNumbers.scrollLeft = 0;
    }

    setupCodeStorge()
    {
        let options = document.getElementById("language-selector").children;
        for (const option of options)
        {
            this.codeMap[option.value] = "";
        }
    }
    
    getCurrentLine() 
    {
        const startPos = this.textarea.selectionStart;
        const value = this.textarea.value;
        let lineStart = value.lastIndexOf('\n', startPos - 1) + 1;
        let lineEnd = value.indexOf('\n', startPos);
        if (lineEnd === -1) lineEnd = value.length;
        return value.substring(lineStart, lineEnd);
    }
    
    insertText(text) 
    {
        const start = this.textarea.selectionStart;
        const end = this.textarea.selectionEnd;
        const value = this.textarea.value;
        
        this.textarea.value = value.substring(0, start) + text + value.substring(end);
        this.textarea.selectionStart = this.textarea.selectionEnd = start + text.length;
        
        this.update();
    }

    update()
    {
        const lines = this.textarea.value.split('\n');
        this.lineNumbers.innerHTML = lines.map((_, i) => i + 1).join('<br>');
        this.textarea.style.height = `${this.lineNumbers.scrollHeight}px`;
        this.editorContainer.style.height = this.textarea.style.height;
        if (typeof hljs !== 'undefined') 
        {
            this.highlight.innerHTML = hljs.highlightAuto(this.textarea.value).value;
            this.highlight.innerHTML += this.textarea.value.endsWith('\n') ? ' ' : '';
            this.highlight.scrollLeft = this.textarea.scrollLeft;
        }
    }
    
    getValue() 
    {
        return this.textarea.value;
    }
    
    setValue(newLanguage) 
    {
        if (this.curLanguage !== newLanguage)
        {
            this.codeMap[this.curLanguage] = this.textarea.value;
            this.textarea.value = this.codeMap[newLanguage];
            this.curLanguage = newLanguage;
            this.update();
        }
    }

    resetValue()
    {
        this.codeMap[this.curLanguage] = "";
        this.textarea.value = "";
        this.update();
    }
}

function escapeStrProcess(inputString)
{
    let strs = inputString.split('^');
    if (strs.length == 1)
    {
        switch (strs[0][0])
        {
        case 'c':
            return ` <code>${strs[0].slice(1)}</code> `;
        case 's':
            return ` <strong>${strs[0].slice(1)}</strong> `;
        }
    }
    let start = "";
    let end = "";
    let ret = ""
    let isEscape = false;
    switch (strs[0][0])
    {
    case 'c':
        strs[0] = strs[0].slice(1);
        start =  ` <code>`;
        end = `</code> `;
        break;
    case 's':
        strs[0] = strs[0].slice(1);
        start =  ` <strong>`;
        end = `</strong> `;
        break;
    }
    ret += start;
    for (const str of strs)
    {
        if (isEscape)
        {
            ret += `<sup>${str}</sup>`;
            isEscape = false;
        }
        else
        {
            ret += str;
            isEscape = true;
        }
    }
    ret += end;
    return ret;
}

function parseStr(inputString)
{
    let strs = inputString.split('$');
    let start = false;
    let ret = "";
    for (const str of strs)
    {
        if (start)
        {
            ret += escapeStrProcess(str);
            start = false;
        }
        else
        {
            start = true;
            ret += str;
        }
    }
    return ret;
}

// 获取页面数据
async function getData()
{
    const queryParams = new URLSearchParams(window.location.search);
    let name = queryParams.get("name");
    const data = await fetch(`/problems/problem/data.json?name=${encodeURIComponent(name)}`, {
        method: "GET", 
        headers: 
        {
            "Content-Type": "application/json"
        },
    })
    if (data.ok === false)
    {
        window.location.href = "/problems/404.html";
    }
    return data.json();
}

// 渲染题目信息
function renderInfo(content)
{
    let parent = document.querySelector(".problem-description-container");
    let element = parent.querySelector(".problem-header");
    let fragment = new DocumentFragment();

    // 渲染题目描述
    let header = document.createElement("h2");
    header.setAttribute("class", "problem-title");
    header.textContent = content.name;
    fragment.appendChild(header);

    let div = document.createElement("div");
    div.setAttribute("class", "problem-meta")
    let span = document.createElement("span");
    switch (content.difficulty)
    {
    case "简单":
        span.setAttribute("class", "badge easy");
        break;
    case "中等":
        span.setAttribute("class", "badge medium");
        break;
    case "困难":
        span.setAttribute("class", "badge hard");
        break;
    }
    span.textContent = content.difficulty;
    div.append(span);
    for (const tag of content.tags)
    {
        span = document.createElement("span");
        span.setAttribute("class", "problem-tag");
        span.textContent = tag;
        div.append(span);
    }
    fragment.appendChild(div);

    element.appendChild(fragment);
    parent = parent.querySelector(".problem-content");
    element = document.getElementById("description");
    element = element.querySelector(".content-text")
    let p;
    for (let para of content.description)
    {
        p = document.createElement("p");
        p.innerHTML = parseStr(para);
        fragment.appendChild(p);
    }

    let count = 1;
    let pre;
    for (let example of content.examples)
    {
        div = document.createElement("div");
        div.setAttribute("class", "sample");
        h = document.createElement("h3");
        h.textContent = `示例 ${count}:`;
        div.appendChild(h);
        pre = document.createElement("pre");
        pre.textContent = example;
        div.appendChild(pre);
        fragment.appendChild(div);
        ++count;
    }

    div = document.createElement("div");
    div.setAttribute("class", "constraints");
    h = document.createElement("h3");
    h.textContent = `提示:`;
    div.appendChild(h);
    let ul = document.createElement("ul");
    let li;
    for (const hint of content.hints)
    {
        li = document.createElement("li");
        li.innerHTML = parseStr(hint);
        ul.appendChild(li);
    }
    div.appendChild(ul);
    fragment.appendChild(div);
    element.appendChild(fragment);

    // 渲染题解
    element = document.getElementById("solutions");
    element = element.querySelector(".content-text");
    for (const solution of content.solutions)
    {
        h = document.createElement("h3");
        h.textContent = solution.title;
        fragment.appendChild(h);
        p = document.createElement("p");
        p.innerHTML = parseStr(solution.description);
        fragment.appendChild(p);
        pre = document.createElement("pre");
        let code = document.createElement("code");
        code.className = 'language-cpp';
        code.textContent = solution.code;
        hljs.highlightElement(code);
        pre.appendChild(code);
        fragment.appendChild(pre);
        p = document.createElement("p");
        p.innerHTML = "<strong>复杂度分析：</strong>";
        fragment.appendChild(p);
        ul = document.createElement("ul");
        li = document.createElement("li");
        li.innerHTML = parseStr(solution.complexity.time);
        ul.appendChild(li);
        li = document.createElement("li");
        li.innerHTML = parseStr(solution.complexity.space);
        ul.appendChild(li);
        fragment.append(ul);
    }
    element.appendChild(fragment);
}

async function initInfo()
{
    const content = await getData();
    renderInfo(content);
    return content;
}

// 提交结果渲染
async function renderResult(response)
{
    let content = await response.json();
    let resultNode = document.getElementById("submissions");
    let result = resultNode.querySelector(".content-text .submission-list");
    let fragment = new DocumentFragment();
    let node = document.createElement("div");
    switch (content.status)
    {
    case "comerr":
        node.textContent = `complie error: ${content.msg}`;
        fragment.appendChild(node);
        break;
    case "runerr":
        node.textContent = `runtime error: ${content.msg}\n`;
        node.textContent += `which: ${content.which}`;
        fragment.appendChild(node);
        break;
    case "wrong":
        node.textContent = `example wrong: ${content.which}`;
        fragment.appendChild(node);
        break;
    case "correct":
        node.textContent = `result correct`;
        fragment.appendChild(node);
        break;
    }
    result.textContent = fragment.textContent;
    const tabButtons = document.querySelectorAll('.tab-btn');
    const tabPanels = document.querySelectorAll('.tab-panel');
    tabButtons.forEach(btn => btn.classList.remove('active'));
    tabPanels.forEach(panel => panel.classList.remove('active'));
    tabButtons[2].classList.add('active');
    tabPanels[2].classList.add('active');
}

// 页面初始化
document.addEventListener('DOMContentLoaded', function() 
{
    // 初始化标签页
    const content = initInfo();

    // 初始化代码编辑器
    const codeEditor = new CodeEditor('code-editor');
    
    // 语言选择切换
    document.getElementById('language-selector').addEventListener('change', function() 
    {
        if (this.value !== "cpp")
        {
            alert("no support");
        }
        codeEditor.setValue(this.value);
    });
    
    // 重置代码按钮
    document.getElementById('reset-btn').addEventListener('click', function() 
    {
        codeEditor.resetValue();
    });

    // 提交代码按钮
    document.getElementById('submit-btn').addEventListener('click', function() 
    {
        if (codeEditor.curLanguage !== "cpp")
        {
            alert("no support");
            return;
        }
        let content = {};
        const queryParams = new URLSearchParams(window.location.search);
        content.name = queryParams.get("name");
        content.code = codeEditor.getValue();
        content.language = codeEditor.curLanguage;
        fetch("/submit", {
            method: "POST", 
            headers: 
            {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(content)
        })
        .then(response => {
            renderResult(response);
        })
    });
    
    // 初始化代码高亮
    if (typeof hljs !== 'undefined') 
    {
        hljs.highlightAll();
    }

    // 初始化标签页切换
    const tabButtons = document.querySelectorAll('.tab-btn');
    const tabPanels = document.querySelectorAll('.tab-panel');
    
    tabButtons.forEach(button => {
        button.addEventListener('click', function() 
        {
            // 移除所有active类
            tabButtons.forEach(btn => btn.classList.remove('active'));
            tabPanels.forEach(panel => panel.classList.remove('active'));
            
            // 添加active类到当前按钮和对应面板
            this.classList.add('active');
            const tabId = this.getAttribute('data-tab');
            document.getElementById(tabId).classList.add('active');
        });
    });
});