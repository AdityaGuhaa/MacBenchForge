window.historyTab = {
    summaries: [],
    
    async init() {
        document.addEventListener('tabChanged', (e) => {
            if (e.detail.target === 'tab-history') {
                this.loadHistory();
            }
        });
        
        const cbAll = document.getElementById('cb-hist-all');
        if (cbAll) {
            cbAll.addEventListener('change', (e) => {
                document.querySelectorAll('.cb-hist-row').forEach(cb => {
                    cb.checked = e.target.checked;
                });
                this.updateCompareBtn();
            });
        }
        
        document.getElementById('btn-compare-runs').addEventListener('click', () => {
            this.compareSelected();
        });
    },

    async loadHistory() {
        try {
            this.summaries = await window.API.getRuns();
            this.renderTable();
        } catch (e) {
            console.error("Failed to load history", e);
        }
    },

    renderTable() {
        const tbody = document.getElementById('history-tbody');
        tbody.innerHTML = '';
        
        if (this.summaries.length === 0) {
            tbody.innerHTML = '<tr><td colspan="10" class="text-center text-secondary">No benchmark runs yet.</td></tr>';
            return;
        }
        
        // Sort newest first by default
        const sorted = [...this.summaries].sort((a,b) => b.run_id - a.run_id);
        
        sorted.forEach(r => {
            const tr = document.createElement('tr');
            
            let statusBadge = '';
            if (r.status === 'done') statusBadge = '<span class="badge success">Done</span>';
            else if (r.status === 'failed') statusBadge = '<span class="badge warning">Failed</span>';
            else statusBadge = '<span class="badge">Running</span>';
            
            tr.innerHTML = `
                <td><input type="checkbox" class="cb-hist-row" value="${r.run_id}" onchange="window.historyTab.updateCompareBtn()"></td>
                <td class="small-text">${formatDate(r.started_at)}</td>
                <td><strong>${r.model_name}</strong></td>
                <td>${r.quant_label}</td>
                <td>${r.preset_name}</td>
                <td style="color:var(--accent); font-weight:bold;">${r.status === 'done' ? formatNumber(r.tokens_per_second) : '-'}</td>
                <td>${r.status === 'done' ? formatNumber(r.time_to_first_token_ms, 0) : '-'}</td>
                <td>${r.status === 'done' ? formatNumber(r.ram_usage_mb, 0) : '-'}</td>
                <td>${statusBadge}</td>
                <td>
                    <button class="btn-icon" onclick="window.historyTab.deleteRun(${r.run_id})" title="Delete Run">🗑️</button>
                </td>
            `;
            tbody.appendChild(tr);
        });
    },

    updateCompareBtn() {
        const selected = document.querySelectorAll('.cb-hist-row:checked');
        const btn = document.getElementById('btn-compare-runs');
        btn.disabled = selected.length < 2;
    },

    compareSelected() {
        const selectedIds = Array.from(document.querySelectorAll('.cb-hist-row:checked')).map(cb => parseInt(cb.value));
        const selectedRuns = this.summaries.filter(s => selectedIds.includes(s.run_id) && s.status === 'done');
        
        if (selectedRuns.length < 2) {
            window.showToast("Select at least 2 completed runs to compare", "warning");
            return;
        }
        
        document.getElementById('comparison-view').classList.remove('hidden');
        
        window.ChartManager.createSpeedChart('chart-speed', selectedRuns);
        window.ChartManager.createRadarChart('chart-radar', selectedRuns);
        
        // Scroll to comparison
        document.getElementById('comparison-view').scrollIntoView({ behavior: 'smooth' });
    },
    
    updateChartsTheme() {
        // Re-render if comparison view is visible
        if (!document.getElementById('comparison-view').classList.contains('hidden')) {
            this.compareSelected();
        }
    },

    async deleteRun(id) {
        if (!confirm("Are you sure you want to delete this run record?")) return;
        
        try {
            await window.API.deleteRun(id);
            window.showToast("Run deleted", "success");
            this.loadHistory();
        } catch (e) {
            window.showToast("Failed to delete run", "error");
        }
    }
};
