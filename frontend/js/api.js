const API = {
    baseUrl: '', // same origin

    async request(endpoint, options = {}) {
        try {
            const response = await fetch(this.baseUrl + endpoint, options);
            if (!response.ok) {
                let errStr = `HTTP Error ${response.status}`;
                try {
                    const errJson = await response.json();
                    if (errJson.error) errStr = errJson.error;
                } catch (e) {}
                throw new Error(errStr);
            }
            return await response.json();
        } catch (error) {
            console.error(`API Error on ${endpoint}:`, error);
            if (window.showToast) {
                window.showToast(error.message, 'error');
            }
            throw error;
        }
    },

    get(endpoint) { return this.request(endpoint); },
    
    post(endpoint, data) {
        return this.request(endpoint, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        });
    },

    del(endpoint) {
        return this.request(endpoint, { method: 'DELETE' });
    },

    // Specific Endpoints
    getHardware() { return this.get('/api/hardware'); },
    getModels() { return this.get('/api/models'); },
    scanModels() { return this.post('/api/models/scan', {}); },
    pickModel() { return this.post('/api/models/pick', {}); },
    
    searchHF(query, page = 0) { 
        const q = query ? `?q=${encodeURIComponent(query)}&page=${page}` : '';
        return this.get('/api/hf/search' + q); 
    },
    getHFFiles(repoId) { return this.get(`/api/hf/files?repo_id=${encodeURIComponent(repoId)}`); },
    startDownload(repoId, filename) { return this.post('/api/hf/download', { repo_id: repoId, filename: filename }); },
    getDownloads() { return this.get('/api/downloads'); },
    getDownloadStatus(id) { return this.get(`/api/downloads/${id}`); },
    
    getConfig() { return this.get('/api/config'); },
    
    startBenchmark(modelId, presetName, promptType) { 
        return this.post('/api/benchmark/run', { model_id: parseInt(modelId), preset_name: presetName, prompt_type: promptType }); 
    },
    getBenchmarkStatus(id) { return this.get(`/api/benchmark/status/${id}`); },
    
    getRuns() { return this.get('/api/runs'); },
    getRun(id) { return this.get(`/api/runs/${id}`); },
    deleteRun(id) { return this.del(`/api/runs/${id}`); },
    
    exportJSON(runIds) { return this.post('/api/export/json', { run_ids: runIds }); },
    exportCSV(runIds) { return this.post('/api/export/csv', { run_ids: runIds }); }
};

window.API = API;
