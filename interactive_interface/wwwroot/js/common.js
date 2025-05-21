// 公共JavaScript函数
function initHeader() 
{
    // 高亮当前页面的导航链接
    const currentPath = window.location.pathname.split('/').pop();
    const navLinks = document.querySelectorAll('.main-nav a');
    
    navLinks.forEach(link => {
        const linkPath = link.getAttribute('href').split('/').pop();
        if (linkPath === currentPath) 
        {
            link.classList.add('active');
        }
    });
}

// 初始化函数
function initCommon() 
{
    initHeader();
}

// 当DOM加载完成时执行
document.addEventListener('DOMContentLoaded', initCommon);