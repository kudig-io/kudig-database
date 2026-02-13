// 侧边栏状态持久化：保存/恢复展开状态和滚动位置，避免每次导航后目录重置
// 改进版：使用 localStorage 替代 sessionStorage，实现跨会话持久化
(function() {
    var STORAGE_KEY = 'mdbook-sidebar-state-v2';
    var SCROLL_KEY = 'mdbook-sidebar-scroll';

    // 获取侧边栏滚动容器
    function getSidebarScrollbox() {
        return document.querySelector('.sidebar-scrollbox');
    }

    // 收集所有展开项的 href 作为唯一标识
    function getExpandedPaths() {
        var paths = [];
        document.querySelectorAll('.sidebar-scrollbox li.chapter-item.expanded').forEach(function(li) {
            var link = li.querySelector(':scope > .chapter-link-wrapper > a[href]');
            if (link) {
                paths.push(link.getAttribute('href'));
            } else {
                var wrapper = li.querySelector(':scope > .chapter-link-wrapper');
                var text = wrapper ? wrapper.textContent.trim().substring(0, 80) : '';
                if (text) paths.push('__text__' + text);
            }
        });
        return paths;
    }

    // 保存当前状态到 localStorage（持久化）
    function saveState() {
        try {
            var scrollbox = getSidebarScrollbox();
            var state = {
                expandedPaths: getExpandedPaths(),
                timestamp: Date.now()
            };
            localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
            
            // 单独保存滚动位置
            if (scrollbox) {
                localStorage.setItem(SCROLL_KEY, scrollbox.scrollTop.toString());
            }
        } catch (e) {
            console.warn('Failed to save sidebar state:', e);
        }
    }

    // 从 localStorage 读取状态
    function loadState() {
        try {
            var data = localStorage.getItem(STORAGE_KEY);
            if (!data) return null;
            
            var state = JSON.parse(data);
            // 清除超过24小时的旧状态
            if (state.timestamp && (Date.now() - state.timestamp > 24 * 60 * 60 * 1000)) {
                localStorage.removeItem(STORAGE_KEY);
                return null;
            }
            return state;
        } catch (e) {
            localStorage.removeItem(STORAGE_KEY);
            return null;
        }
    }

    // 加载滚动位置
    function loadScrollPosition() {
        try {
            var scroll = localStorage.getItem(SCROLL_KEY);
            return scroll ? parseInt(scroll, 10) : 0;
        } catch (e) {
            return 0;
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

        // 构建 href → true 的映射，避免多次数组查找
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
            if (el && el.classList && el.classList.contains('sidebar-scrollbox')) break;
        }
    }

    // 监听用户点击章节项，展开/折叠时保存状态
    function attachClickListeners() {
        document.querySelectorAll('.sidebar-scrollbox .chapter-item').forEach(function(li) {
            li.addEventListener('click', function(e) {
                // 延迟保存，确保 expanded 类已更新
                setTimeout(saveState, 100);
            });
        });
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

        // 恢复滚动位置
        var scrollTop = loadScrollPosition();
        if (scrollTop) {
            setTimeout(function() {
                restoreScrollPosition(scrollTop);
            }, 100);
        }

        // 附加点击监听器
        attachClickListeners();
    }

    // 注册多个保存时机
    window.addEventListener('beforeunload', saveState);
    
    // 监听链接点击，在页面跳转前保存
    document.addEventListener('click', function(e) {
        var link = e.target.closest('a[href]');
        if (link && link.closest('.sidebar-scrollbox')) {
            saveState();
        }
    });

    // 定期自动保存（每5秒）
    setInterval(saveState, 5000);

    // 在 DOM 就绪后初始化
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', function() {
            setTimeout(init, 100);
        });
    } else {
        setTimeout(init, 100);
    }

    // 兜底：延迟执行确保 mdBook 动态渲染完成后状态正确
    setTimeout(function() {
        expandActiveAncestors();
        var scrollTop = loadScrollPosition();
        if (scrollTop) {
            restoreScrollPosition(scrollTop);
        }
    }, 500);
})();
