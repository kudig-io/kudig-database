// 页面加载完成后，强制折叠所有侧边栏章节
(function() {
    function collapseAll() {
        document.querySelectorAll('.sidebar-scrollbox li.chapter-item.expanded').forEach(function(li) {
            li.classList.remove('expanded');
        });
    }

    // 多重时机确保折叠生效
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', function() {
            collapseAll();
            requestAnimationFrame(collapseAll);
        });
    } else {
        collapseAll();
        requestAnimationFrame(collapseAll);
    }

    // 兜底：延迟执行一次，确保 mdBook 动态渲染完成后仍然折叠
    setTimeout(collapseAll, 100);
    setTimeout(collapseAll, 300);
})();
