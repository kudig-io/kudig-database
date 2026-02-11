// 侧边栏状态持久化：保存/恢复展开状态和滚动位置，避免每次导航后目录重置
(function() {
    var STORAGE_KEY = 'mdbook-sidebar-state';

    // 获取侧边栏滚动容器
    function getSidebarScrollbox() {
        return document.querySelector('.sidebar-scrollbox');
    }

    // 收集所有展开项的 href 作为唯一标识
    function getExpandedPaths() {
        var paths = [];
        document.querySelectorAll('.sidebar-scrollbox li.chapter-item.expanded').forEach(function(li) {
            // <a> 在 <span class="chapter-link-wrapper"> 内，不是 <li> 的直接子元素
            var link = li.querySelector(':scope > .chapter-link-wrapper > a[href]');
            if (link) {
                paths.push(link.getAttribute('href'));
            } else {
                // 没有链接的章节项（如分组标题），用文本内容作为标识
                var wrapper = li.querySelector(':scope > .chapter-link-wrapper');
                var text = wrapper ? wrapper.textContent.trim().substring(0, 80) : '';
                if (text) paths.push('__text__' + text);
            }
        });
        return paths;
    }

    // 保存当前状态到 sessionStorage
    function saveState() {
        try {
            var scrollbox = getSidebarScrollbox();
            var state = {
                expandedPaths: getExpandedPaths(),
                scrollTop: scrollbox ? scrollbox.scrollTop : 0
            };
            sessionStorage.setItem(STORAGE_KEY, JSON.stringify(state));
        } catch (e) {
            // sessionStorage 不可用时静默忽略
        }
    }

    // 从 sessionStorage 读取状态
    function loadState() {
        try {
            var data = sessionStorage.getItem(STORAGE_KEY);
            return data ? JSON.parse(data) : null;
        } catch (e) {
            sessionStorage.removeItem(STORAGE_KEY);
            return null;
        }
    }

    // 折叠所有项（首次访问时的默认行为）
    function collapseAll() {
        document.querySelectorAll('.sidebar-scrollbox li.chapter-item.expanded').forEach(function(li) {
            li.classList.remove('expanded');
        });
    }

    // 根据保存的 href 列表恢复展开状态
    function restoreExpandedState(expandedPaths) {
        if (!expandedPaths || !expandedPaths.length) return;

        // 先折叠所有
        collapseAll();

        // 构建 href → li 的映射，避免多次 DOM 查询
        var pathSet = {};
        for (var i = 0; i < expandedPaths.length; i++) {
            pathSet[expandedPaths[i]] = true;
        }

        document.querySelectorAll('.sidebar-scrollbox li.chapter-item').forEach(function(li) {
            var link = li.querySelector(':scope > .chapter-link-wrapper > a[href]');
            if (link && pathSet[link.getAttribute('href')]) {
                li.classList.add('expanded');
            } else if (!link) {
                var wrapper = li.querySelector(':scope > .chapter-link-wrapper');
                var text = wrapper ? wrapper.textContent.trim().substring(0, 80) : '';
                if (text && pathSet['__text__' + text]) {
                    li.classList.add('expanded');
                }
            }
        });
    }

    // 恢复滚动位置
    function restoreScrollPosition(scrollTop) {
        var scrollbox = getSidebarScrollbox();
        if (scrollbox && scrollTop) {
            scrollbox.scrollTop = scrollTop;
        }
    }

    // 确保当前活动页面的所有祖先章节展开
    function expandActiveAncestors() {
        var active = document.querySelector('.sidebar-scrollbox a.active');
        if (!active) return;

        var el = active.parentElement;
        while (el) {
            if (el.classList && el.classList.contains('chapter-item')) {
                el.classList.add('expanded');
            }
            el = el.parentElement;
            // 到达 sidebar-scrollbox 就停止
            if (el && el.classList && el.classList.contains('sidebar-scrollbox')) break;
        }
    }

    // 主初始化
    function init() {
        var saved = loadState();

        if (saved && saved.expandedPaths && saved.expandedPaths.length > 0) {
            // 有保存状态：恢复展开项
            restoreExpandedState(saved.expandedPaths);
        } else {
            // 首次访问：折叠所有
            collapseAll();
        }

        // 始终确保当前页面在侧边栏中可见
        expandActiveAncestors();

        // 恢复滚动位置（延迟执行以确保 DOM 布局完成）
        if (saved && saved.scrollTop) {
            setTimeout(function() {
                restoreScrollPosition(saved.scrollTop);
            }, 50);
        }
    }

    // 注册页面卸载前保存状态
    window.addEventListener('beforeunload', saveState);

    // 在 DOM 就绪后初始化，使用多重时机确保 mdBook 渲染完成
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', function() {
            requestAnimationFrame(init);
        });
    } else {
        requestAnimationFrame(init);
    }

    // 兜底：延迟执行确保 mdBook 动态渲染完成后状态正确
    setTimeout(function() {
        var saved = loadState();
        if (saved && saved.scrollTop) {
            restoreScrollPosition(saved.scrollTop);
        }
    }, 300);
})();
