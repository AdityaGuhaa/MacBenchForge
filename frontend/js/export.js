window.exportTab = {
    async init() {
        document.addEventListener('tabChanged', (e) => {
            if (e.detail.target === 'tab-export') {
                this.loadRuns();
            }
        });
        
        document.getElementById('btn-export-sel-all').addEventListener('click', () => {
            document.querySelectorAll('.cb-export').forEach(cb => cb.checked = true);
        });
        document.getElementById('btn-export-desel-all').addEventListener('click', () => {
            document.querySelectorAll('.cb-export').forEach(cb => cb.checked = false);
        });
        
        document.getElementById('btn-export-json').addEventListener('click', () => this.doExport('json'));
        document.getElementById('btn-export-csv').addEventListener('click', () => this.doExport('csv'));
    },

    async loadRuns() {
        try {
            const runs = await window.API.getRuns();
            const container = document.getElementById('export-list');
            container.innerHTML = '';
            
            if (runs.length === 0) {
                container.innerHTML = '<div class="text-secondary">No runs available to export.</div>';
                return;
            }
            
            // Only export done runs
            const doneRuns = runs.filter(r => r.status === 'done');
            
            doneRuns.forEach(r => {
                const label = document.createElement('label');
                label.style.display = 'block';
                label.style.padding = '0.5rem';
                label.style.borderBottom = '1px solid var(--border-glass)';
                label.style.cursor = 'pointer';
                
                label.innerHTML = `
                    <input type="checkbox" class="cb-export" value="${r.run_id}" checked>
                    <strong>${r.model_name}</strong> (${r.quant_label}) 
                    <span class="text-secondary">— ${formatNumber(r.tokens_per_second)} t/s — ${formatDate(r.started_at)}</span>
                `;
                container.appendChild(label);
            });
        } catch (e) {
            console.error("Failed to load runs for export", e);
        }
    },

    async doExport(format) {
        const selected = Array.from(document.querySelectorAll('.cb-export:checked')).map(cb => parseInt(cb.value));
        if (selected.length === 0) {
            window.showToast("Please select at least one run to export.", "warning");
            return;
        }
        
        try {
            let res;
            if (format === 'json') res = await window.API.exportJSON(selected);
            else res = await window.API.exportCSV(selected);
            
            window.showToast(`Exported to: ${res.path}`, "success");
        } catch (e) {
            window.showToast(`Export failed`, "error");
        }
    }
};
