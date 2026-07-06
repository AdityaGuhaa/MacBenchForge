window.ChartManager = {
    instances: {},

    getColors() {
        const isLight = document.body.classList.contains('light-theme');
        return {
            text: isLight ? '#5a5a6a' : '#a0a0b0',
            grid: isLight ? 'rgba(0,0,0,0.05)' : 'rgba(255,255,255,0.05)',
            accents: ['#6c63ff', '#2ecc71', '#f1c40f', '#e74c3c', '#9b59b6', '#3498db']
        };
    },

    setupDefaults() {
        const c = this.getColors();
        Chart.defaults.color = c.text;
        Chart.defaults.font.family = "'Inter', sans-serif";
    },
    
    destroy(id) {
        if (this.instances[id]) {
            this.instances[id].destroy();
            delete this.instances[id];
        }
    },

    createSpeedChart(canvasId, summaries) {
        this.setupDefaults();
        this.destroy(canvasId);
        
        const c = this.getColors();
        const ctx = document.getElementById(canvasId).getContext('2d');
        
        const labels = summaries.map(s => `${s.model_name} (${s.quant_label})`);
        const promptData = summaries.map(s => s.prompt_tokens_per_sec);
        const genData = summaries.map(s => s.gen_tokens_per_sec);

        this.instances[canvasId] = new Chart(ctx, {
            type: 'bar',
            data: {
                labels: labels,
                datasets: [
                    {
                        label: 'Prompt Eval (t/s)',
                        data: promptData,
                        backgroundColor: c.accents[5],
                        borderRadius: 4
                    },
                    {
                        label: 'Generation (t/s)',
                        data: genData,
                        backgroundColor: c.accents[0],
                        borderRadius: 4
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: { position: 'top' },
                    title: { display: true, text: 'Tokens per Second (Prompt vs Gen)' }
                },
                scales: {
                    y: { grid: { color: c.grid } },
                    x: { grid: { display: false } }
                }
            }
        });
    },

    createRadarChart(canvasId, summaries) {
        this.setupDefaults();
        this.destroy(canvasId);
        
        const c = this.getColors();
        const ctx = document.getElementById(canvasId).getContext('2d');
        
        // Normalize data for radar chart (0-100 scale)
        // Find max values
        let maxTps = 0, maxPrompt = 0, maxTtft = 0, maxRam = 0;
        summaries.forEach(s => {
            if (s.gen_tokens_per_sec > maxTps) maxTps = s.gen_tokens_per_sec;
            if (s.prompt_tokens_per_sec > maxPrompt) maxPrompt = s.prompt_tokens_per_sec;
            if (s.time_to_first_token_ms > maxTtft) maxTtft = s.time_to_first_token_ms;
            if (s.ram_usage_mb > maxRam) maxRam = s.ram_usage_mb;
        });
        
        // Prevent div by zero
        maxTps = maxTps || 1; maxPrompt = maxPrompt || 1; maxTtft = maxTtft || 1; maxRam = maxRam || 1;

        const datasets = summaries.map((s, i) => {
            const color = c.accents[i % c.accents.length];
            return {
                label: `${s.model_name} (${s.quant_label})`,
                data: [
                    (s.gen_tokens_per_sec / maxTps) * 100,
                    (s.prompt_tokens_per_sec / maxPrompt) * 100,
                    100 - ((s.time_to_first_token_ms / maxTtft) * 100), // Lower is better for TTFT
                    100 - ((s.ram_usage_mb / maxRam) * 100)           // Lower is better for RAM
                ],
                backgroundColor: color.startsWith('#') 
                    ? color + '33'  // Append 33 (hex for ~20% opacity) to hex color
                    : color.replace(')', ', 0.2)').replace('rgb', 'rgba'),
                borderColor: color,
                pointBackgroundColor: color
            };
        });

        this.instances[canvasId] = new Chart(ctx, {
            type: 'radar',
            data: {
                labels: ['Generation Speed', 'Prompt Eval Speed', 'Responsiveness (TTFT)', 'Memory Efficiency'],
                datasets: datasets
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    r: {
                        angleLines: { color: c.grid },
                        grid: { color: c.grid },
                        pointLabels: { color: c.text, font: { size: 11 } },
                        ticks: { display: false } // Hide numbers on web
                    }
                }
            }
        });
    }
};
