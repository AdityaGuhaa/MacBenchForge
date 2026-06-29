// Utility Formatters
function formatBytes(bytes) {
    if (bytes === 0) return '0 B';
    if (!bytes) return '-';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function formatDate(isoString) {
    if (!isoString) return '-';
    const d = new Date(isoString);
    return d.toLocaleString();
}

function formatNumber(n, decimals = 2) {
    if (n === null || n === undefined) return '-';
    return Number(n).toFixed(decimals);
}

// Toast Notifications
window.showToast = function(message, type = 'info') {
    const container = document.getElementById('toast-container');
    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.textContent = message;
    container.appendChild(toast);
    
    setTimeout(() => {
        toast.style.opacity = '0';
        setTimeout(() => toast.remove(), 300);
    }, 3000);
};

// Main App Initialization
document.addEventListener('DOMContentLoaded', async () => {
    
    // Theme setup
    const themeBtn = document.getElementById('theme-toggle');
    const savedTheme = localStorage.getItem('theme') || 'dark';
    if (savedTheme === 'light') {
        document.body.classList.add('light-theme');
        themeBtn.textContent = '☀️';
    }
    
    themeBtn.addEventListener('click', () => {
        document.body.classList.toggle('light-theme');
        const isLight = document.body.classList.contains('light-theme');
        localStorage.setItem('theme', isLight ? 'light' : 'dark');
        themeBtn.textContent = isLight ? '☀️' : '🌙';
        
        // Re-render charts if they exist
        if (window.historyTab && window.historyTab.updateChartsTheme) {
            window.historyTab.updateChartsTheme();
        }
    });

    // Tab Navigation
    const tabs = document.querySelectorAll('.tab-btn');
    const panels = document.querySelectorAll('.tab-panel');
    
    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            tabs.forEach(t => t.classList.remove('active'));
            panels.forEach(p => p.classList.remove('active'));
            
            tab.classList.add('active');
            const target = tab.getAttribute('data-target');
            document.getElementById(target).classList.add('active');
            
            // Notify module if needed
            const event = new CustomEvent('tabChanged', { detail: { target } });
            document.dispatchEvent(event);
        });
    });

    // Load initial data
    try {
        const hw = await window.API.getHardware();
        document.getElementById('hw-chip').textContent = hw.chip_name;
        document.getElementById('hw-cores').textContent = `${hw.total_cores} CPU Cores`;
        document.getElementById('hw-gpu').textContent = `${hw.gpu_cores} GPU Cores`;
        const vramGb = (hw.memory_bytes * 0.75) / (1024 * 1024 * 1024);
        document.getElementById('hw-ram').textContent = `${hw.memory_label} (${vramGb.toFixed(1)} GB VRAM Available)`;
        
        // Save globally
        window.appData = { hw: hw };
        
        // Load config
        window.appData.config = await window.API.getConfig();
        
        // Check first run wizard
        if (window.wizardTab) window.wizardTab.checkFirstRun();
        
        // Init modules
        if (window.benchmarkTab) window.benchmarkTab.init();
        if (window.hfTab) window.hfTab.init();
        if (window.historyTab) window.historyTab.init();
        if (window.exportTab) window.exportTab.init();
        
        loadDashboardStats();
        
    } catch (e) {
        console.error("Initialization failed", e);
    }
    
    // Models Tab Logic (Simple enough to keep here)
    const scanBtn = document.getElementById('btn-scan-models');
    if (scanBtn) {
        scanBtn.addEventListener('click', async () => {
            scanBtn.textContent = 'Scanning...';
            scanBtn.disabled = true;
            try {
                const res = await window.API.scanModels();
                window.showToast(`Found ${res.count} models`, 'success');
                loadModelsGrid();
            } finally {
                scanBtn.textContent = 'Scan Directory';
                scanBtn.disabled = false;
            }
        });
    }

    const pickBtn = document.getElementById('btn-pick-model');
    if (pickBtn) {
        pickBtn.addEventListener('click', async () => {
            pickBtn.textContent = 'Picking...';
            pickBtn.disabled = true;
            try {
                const res = await window.API.pickModel();
                if (res.ok) {
                    window.showToast("Model added successfully!", "success");
                    loadModelsGrid();
                }
            } catch (e) {
                // If cancelled or error, toast is handled
            } finally {
                pickBtn.textContent = 'Pick from Finder';
                pickBtn.disabled = false;
            }
        });
    }
    
    document.dispatchEvent(new CustomEvent('tabChanged', { detail: { target: 'tab-dashboard' } }));
});

// Models Grid Loading
async function loadModelsGrid() {
    const grid = document.getElementById('models-grid');
    if (!grid) return;
    
    try {
        const models = await window.API.getModels();
        grid.innerHTML = '';
        
        if (models.length === 0) {
            grid.innerHTML = '<div class="text-secondary p-2">No models found. Click Scan Directory.</div>';
            return;
        }
        
        models.forEach(m => {
            const card = document.createElement('div');
            card.className = 'model-card glass-card';
            card.innerHTML = `
                <div class="model-title">${m.name}</div>
                <div class="model-meta">
                    <span class="badge accent">${m.size_label}</span>
                    <span class="badge">${m.quant_label}</span>
                </div>
                <div class="small-text text-secondary mt-1" style="word-break: break-all;">
                    ${m.path}
                </div>
            `;
            grid.appendChild(card);
        });
    } catch (e) {
        grid.innerHTML = '<div class="text-secondary">Failed to load models.</div>';
    }
}

// Dashboard Loading
async function loadDashboardStats() {
    try {
        const models = await window.API.getModels();
        document.getElementById('stat-models-count').textContent = models.length;
        
        const runs = await window.API.getRuns();
        document.getElementById('stat-runs-count').textContent = runs.length;
        
        let best = 0;
        runs.forEach(r => {
            if (r.status === 'done' && r.tokens_per_second > best) best = r.tokens_per_second;
        });
        document.getElementById('stat-best-speed').textContent = best > 0 ? `${formatNumber(best)} t/s` : '-';
        
        // Recent runs
        const list = document.getElementById('dashboard-recent-list');
        if (runs.length === 0) {
            list.innerHTML = '<div class="text-secondary mt-1">No runs yet.</div>';
        } else {
            const sorted = runs.sort((a, b) => new Date(b.started_at) - new Date(a.started_at)).slice(0, 5);
            let html = '<ul style="list-style: none; margin-top: 1rem;">';
            sorted.forEach(r => {
                const color = r.status === 'done' ? 'var(--success)' : (r.status==='failed'?'var(--danger)':'var(--warning)');
                html += `
                    <li style="padding: 0.5rem 0; border-bottom: 1px solid var(--border-glass); display: flex; justify-content: space-between;">
                        <span>${r.model_name} <span class="badge" style="margin-left: 0.5rem">${r.preset_name}</span></span>
                        <span style="color: ${color}; font-weight: 600;">${r.status === 'done' ? formatNumber(r.tokens_per_second) + ' t/s' : r.status}</span>
                    </li>
                `;
            });
            html += '</ul>';
            list.innerHTML = html;
        }
        
    } catch (e) { }
}

document.addEventListener('tabChanged', (e) => {
    if (e.detail.target === 'tab-models') loadModelsGrid();
    if (e.detail.target === 'tab-dashboard') loadDashboardStats();
});
