window.benchmarkTab = {
    pollInterval: null,
    currentRunId: null,

    async init() {
        document.addEventListener('tabChanged', (e) => {
            if (e.detail.target === 'tab-benchmark') {
                this.loadOptions();
            }
        });
        
        document.getElementById('btn-start-benchmark').addEventListener('click', () => this.startBenchmark());
    },

    async loadOptions() {
        try {
            const models = await window.API.getModels();
            const config = window.appData.config;
            
            const mSelect = document.getElementById('bm-model-select');
            mSelect.innerHTML = '';
            models.forEach(m => {
                const opt = document.createElement('option');
                opt.value = m.id;
                opt.textContent = `${m.name} (${m.quant_label} - ${m.size_label})`;
                mSelect.appendChild(opt);
            });

            const pSelect = document.getElementById('bm-preset-select');
            if (pSelect.children.length === 0) {
                pSelect.innerHTML = `
                    <option value="quick">Quick (Fast, low variance)</option>
                    <option value="thorough">Thorough (Longer, more accurate)</option>
                `;
            }

            const promptSelect = document.getElementById('bm-prompt-select');
            if (promptSelect.children.length === 0 && config && config.prompt_presets) {
                promptSelect.innerHTML = '';
                config.prompt_presets.forEach(p => {
                    const opt = document.createElement('option');
                    opt.value = p.id;
                    opt.textContent = p.label;
                    promptSelect.appendChild(opt);
                });
            }
        } catch (e) {
            console.error("Failed to load benchmark options", e);
        }
    },

    async startBenchmark() {
        const mSelect = document.getElementById('bm-model-select');
        const pSelect = document.getElementById('bm-preset-select');
        const promptSelect = document.getElementById('bm-prompt-select');

        if (!mSelect.value) {
            window.showToast("Please select a model first", "error");
            return;
        }

        const btn = document.getElementById('btn-start-benchmark');
        btn.disabled = true;
        btn.textContent = 'Starting...';
        
        document.getElementById('bm-status-text').textContent = "Initializing...";
        document.getElementById('bm-progress-container').classList.remove('hidden');
        document.getElementById('bm-results-grid').classList.add('hidden');
        document.getElementById('bm-progress-bar').style.width = '100%';

        try {
            // Because our backend implementation fires async but we didn't return the ID cleanly (or maybe we did), 
            // wait, we returned {status: "started"}. Oh wait, the backend doesn't return run_id! 
            // We need to poll /api/runs to find the latest "pending" or "running" run.
            await window.API.startBenchmark(mSelect.value, pSelect.value, promptSelect.value);
            
            // Wait a sec for the DB insert
            setTimeout(() => this.findAndPollRun(), 1000);
            
        } catch (e) {
            btn.disabled = false;
            btn.textContent = 'Start Benchmark';
            document.getElementById('bm-status-text').textContent = "Failed to start.";
            document.getElementById('bm-progress-container').classList.add('hidden');
        }
    },
    
    async findAndPollRun() {
        try {
            const runs = await window.API.getRuns();
            // Sort by ID descending
            runs.sort((a,b) => b.run_id - a.run_id);
            if (runs.length > 0) {
                this.currentRunId = runs[0].run_id;
                this.pollStatus();
            }
        } catch (e) {
            // retry
            setTimeout(() => this.findAndPollRun(), 2000);
        }
    },

    pollStatus() {
        if (this.pollInterval) clearInterval(this.pollInterval);
        
        this.pollInterval = setInterval(async () => {
            try {
                const run = await window.API.getBenchmarkStatus(this.currentRunId);
                const statusText = document.getElementById('bm-status-text');
                
                if (run.status === 'done') {
                    clearInterval(this.pollInterval);
                    statusText.textContent = "Benchmark Complete!";
                    document.getElementById('bm-progress-container').classList.add('hidden');
                    this.displayResults(run);
                    this.resetBtn();
                } else if (run.status === 'failed') {
                    clearInterval(this.pollInterval);
                    statusText.textContent = "Benchmark Failed.";
                    document.getElementById('bm-progress-container').classList.add('hidden');
                    window.showToast("Benchmark failed to run", "error");
                    this.resetBtn();
                } else {
                    statusText.textContent = `Running benchmark (${run.preset_name})... This may take a while.`;
                }
            } catch (e) {
                // Ignore transient errors during polling
            }
        }, 2000);
    },
    
    displayResults(run) {
        document.getElementById('bm-results-grid').classList.remove('hidden');
        document.getElementById('res-overall').textContent = formatNumber(run.tokens_per_second);
        document.getElementById('res-prompt').textContent = formatNumber(run.prompt_tokens_per_sec);
        document.getElementById('res-gen').textContent = formatNumber(run.gen_tokens_per_sec);
        document.getElementById('res-ttft').textContent = formatNumber(run.time_to_first_token_ms, 0);
        document.getElementById('res-ram').textContent = formatNumber(run.ram_usage_mb, 0);
    },
    
    resetBtn() {
        const btn = document.getElementById('btn-start-benchmark');
        btn.disabled = false;
        btn.textContent = 'Run Another';
    }
};
