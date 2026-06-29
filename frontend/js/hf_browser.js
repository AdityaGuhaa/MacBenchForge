window.hfTab = {
    searchTimeout: null,
    pollInterval: null,

    async init() {
        document.addEventListener('tabChanged', (e) => {
            if (e.detail.target === 'tab-huggingface') {
                this.loadCurated();
            }
        });

        const searchInput = document.getElementById('hf-search-input');
        searchInput.addEventListener('input', (e) => {
            clearTimeout(this.searchTimeout);
            this.searchTimeout = setTimeout(() => {
                this.searchHF(e.target.value);
            }, 500);
        });

        document.getElementById('btn-close-hf-modal').addEventListener('click', () => {
            document.getElementById('modal-hf-files').classList.add('hidden');
        });
    },

    async loadCurated() {
        // If search has text, don't load curated
        if (document.getElementById('hf-search-input').value.trim() !== '') return;
        
        try {
            const results = await window.API.searchHF('');
            this.renderResults(results, true);
        } catch (e) {
            console.error("Failed to load curated models", e);
        }
    },

    async searchHF(query) {
        if (!query.trim()) {
            this.loadCurated();
            return;
        }

        const container = document.getElementById('hf-results-container');
        container.innerHTML = '<div class="text-secondary p-2">Searching...</div>';

        try {
            const results = await window.API.searchHF(query);
            this.renderResults(results, false);
        } catch (e) {
            container.innerHTML = '<div class="text-secondary p-2">Search failed.</div>';
        }
    },

    renderResults(results, isCurated) {
        const container = document.getElementById('hf-results-container');
        container.innerHTML = '';
        
        if (results.length === 0) {
            container.innerHTML = '<div class="text-secondary p-2">No GGUF models found.</div>';
            return;
        }

        results.forEach(r => {
            const card = document.createElement('div');
            card.className = 'model-card glass-card';
            
            let badges = '';
            if (r.is_curated) badges += '<span class="badge accent">⭐ Curated</span> ';
            badges += `<span class="badge">⬇️ ${formatNumber(r.downloads, 0)}</span>`;
            
            card.innerHTML = `
                <div class="model-title" style="cursor: pointer;" onclick="window.hfTab.showFiles('${r.repo_id}')">
                    ${r.author}/<span class="accent-text">${r.model_name}</span>
                </div>
                <div class="model-meta mt-1">${badges}</div>
                <button class="btn btn-secondary mt-1 w-100" onclick="window.hfTab.showFiles('${r.repo_id}')">View Files</button>
            `;
            container.appendChild(card);
        });
    },

    async showFiles(repoId) {
        document.getElementById('modal-hf-title').textContent = repoId;
        const tbody = document.getElementById('hf-files-tbody');
        tbody.innerHTML = '<tr><td colspan="5" class="text-center">Loading files...</td></tr>';
        
        document.getElementById('modal-hf-files').classList.remove('hidden');
        document.getElementById('download-progress-container').classList.add('hidden');

        try {
            const files = await window.API.getHFFiles(repoId);
            tbody.innerHTML = '';
            
            if (files.length === 0) {
                tbody.innerHTML = '<tr><td colspan="5" class="text-center">No GGUF files found.</td></tr>';
                return;
            }
            
            files.forEach(f => {
                const tr = document.createElement('tr');
                
                let comp = f.fits_in_memory ? '<span class="badge success">Fits in RAM</span>' : '<span class="badge warning">Too Large</span>';
                if (f.is_recommended) comp += ' <span class="badge accent">Recommended</span>';
                
                tr.innerHTML = `
                    <td>${f.filename}</td>
                    <td>${f.size_label}</td>
                    <td>${f.quant_label}</td>
                    <td>${comp}</td>
                    <td><button class="btn btn-primary" onclick="window.hfTab.startDownload('${repoId}', '${f.filename}')">Download</button></td>
                `;
                tbody.appendChild(tr);
            });
        } catch (e) {
            tbody.innerHTML = '<tr><td colspan="5" class="text-center text-secondary">Failed to load files.</td></tr>';
        }
    },

    async startDownload(repoId, filename) {
        try {
            const res = await window.API.startDownload(repoId, filename);
            document.getElementById('download-progress-container').classList.remove('hidden');
            this.pollDownload(res.job_id);
        } catch (e) {
            window.showToast("Failed to start download", "error");
        }
    },

    pollDownload(jobId) {
        if (this.pollInterval) clearInterval(this.pollInterval);
        
        const pb = document.getElementById('dl-progress-bar');
        const st = document.getElementById('dl-status-text');
        
        this.pollInterval = setInterval(async () => {
            try {
                const job = await window.API.getDownloadStatus(jobId);
                
                if (job.status === 'done') {
                    clearInterval(this.pollInterval);
                    pb.style.width = '100%';
                    pb.style.background = 'var(--success)';
                    st.textContent = "Download Complete!";
                    window.showToast(`${job.filename} downloaded successfully!`, 'success');
                    
                    // Auto-scan
                    window.API.scanModels();
                    
                    setTimeout(() => {
                        document.getElementById('modal-hf-files').classList.add('hidden');
                        pb.style.background = 'var(--accent)'; // reset
                    }, 2000);
                } else if (job.status === 'failed') {
                    clearInterval(this.pollInterval);
                    pb.style.background = 'var(--danger)';
                    st.textContent = `Error: ${job.error}`;
                } else {
                    if (job.total_bytes > 0) {
                        const pct = (job.downloaded_bytes / job.total_bytes) * 100;
                        pb.style.width = `${pct}%`;
                        st.textContent = `Downloading... ${formatBytes(job.downloaded_bytes)} / ${formatBytes(job.total_bytes)}`;
                    } else {
                        st.textContent = "Starting download...";
                        pb.style.width = '5%';
                    }
                }
            } catch (e) {}
        }, 1000);
    }
};
