// 题库列表页面逻辑
function initToolList(list) 
{
    // 搜索功能
    const searchInput = document.querySelector('.search-input');
    if (searchInput) 
    {
        searchInput.addEventListener('input', function() 
        {
            const searchTerm = this.value.toLowerCase();
            const rows = document.querySelectorAll('.problem-list tbody tr');
            rows.forEach(row => {
                const title = row.querySelector('td:nth-child(2)').textContent.toLowerCase();
                if (title.includes(searchTerm)) 
                {
                    row.style.display = '';
                } 
                else 
                {
                    row.style.display = 'none';
                }
            });
        });
    }
    
    // 难度筛选
    const difficultySelect = document.querySelector('.difficulty-select');
    if (difficultySelect) 
    {
        difficultySelect.addEventListener('change', function() 
        {
            const selectedDifficulty = this.value;
            const rows = document.querySelectorAll('.problem-list tbody tr');
            rows.forEach(row => {
                const difficulty = row.querySelector('.badge').classList[1];
                if (!selectedDifficulty || difficulty === selectedDifficulty) 
                {
                    row.style.display = '';
                } 
                else 
                {
                    row.style.display = 'none';
                }
            });
        });
    }
    
    // 标签筛选
    const tagSelect = document.querySelector('.tag-select');
    if (tagSelect) 
    {
        tagSelect.addEventListener('change', function() 
        {
            let isSelected = false;
            const selectedtag = this.value;
            const rows = document.querySelectorAll('.problem-list tbody tr');
            rows.forEach(row => {
                const tags = row.querySelectorAll('.problem-tag');
                tags.forEach(tag => {
                    if (!selectedtag || tag.textContent === selectedtag) 
                    {
                        isSelected = true;
                        return;
                    }
                })
                if (isSelected) 
                {
                    row.style.display = '';
                } 
                else 
                {
                    row.style.display = 'none';
                }
            });
        });
    }
}

async function getProblemList()
{
    const data = await fetch("/problems/data.json", { 
        method: "GET",
        headers: 
        {
            "Content-Type": "application/json",
        }
    });
    return data.json();
}

function initTagList(newTag)
{
    let tagList = document.getElementsByClassName("tag-select")[0];
    const tags = tagList.children;
    let exist = false;
    for (const tag of tags) 
    {
        if (tag.value === newTag) 
        {
            exist = true;
            break;
        }
    }
    if (!exist)
    {
        let option = document.createElement("option");
        option.value = newTag;
        option.textContent = newTag;
        tagList.appendChild(option);
    }
}

function initProblemList(list)
{
    let index = 1;
    let fragment = new DocumentFragment();
    let element;
    list.forEach((problem) => {
        let tr = document.createElement("tr");
        let td = document.createElement("td");
        td.textContent = index;
        tr.appendChild(td)
        td = document.createElement("td");
        element = document.createElement("a");
        element.setAttribute("href", `/problems/problem.html?name=${problem.name}`);
        element.textContent = problem.name;
        td.appendChild(element);
        tr.appendChild(td);
        td = document.createElement("td");
        for (let i = 0; i < problem.tags.length; ++i)
        {
            element = document.createElement("span");
            element.setAttribute("class", "problem-tag");
            element.textContent = problem.tags[i];
            initTagList(problem.tags[i]);
            td.appendChild(element);
        }
        tr.appendChild(td);
        td = document.createElement("td");
        element = document.createElement("span");
        switch (problem.difficulty)
        {
        case "简单":
            element.setAttribute("class", "badge easy");
            break;
        case "中等":
            element.setAttribute("class", "badge medium");
            break;
        case "困难":
            element.setAttribute("class", "badge hard");
            break;
        }
        element.textContent = problem.difficulty;
        td.appendChild(element);
        tr.appendChild(td);
        fragment.appendChild(tr);
    })
    document.querySelector('.problem-list tbody').appendChild(fragment);
}

// 初始化题库页面
document.addEventListener('DOMContentLoaded', async function() 
{
    const list = await getProblemList();
    initProblemList(list)
    initToolList(list);
});