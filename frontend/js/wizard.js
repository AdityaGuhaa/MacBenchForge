window.wizardTab = {
    async checkFirstRun() {
        // Skip if already dismissed
        if (localStorage.getItem('wizard_dismissed')) return;
        
        try {
            // If there are no models and no runs, probably first time
            const models = await window.API.getModels();
            const runs = await window.API.getRuns();
            
            if (models.length === 0 && runs.length === 0) {
                this.showWizard();
            }
        } catch (e) {}
    },

    showWizard() {
        const modal = document.getElementById('modal-wizard');
        modal.classList.remove('hidden');
        
        // Fill HW info
        if (window.appData && window.appData.hw) {
            const hw = window.appData.hw;
            document.getElementById('wiz-hw-desc').textContent = 
                `${hw.chip_name} with ${hw.memory_label} Unified Memory`;
        }
        
        document.getElementById('btn-wizard-done').addEventListener('click', () => {
            modal.classList.add('hidden');
            localStorage.setItem('wizard_dismissed', 'true');
            
            // Go to HuggingFace tab to download a model
            document.querySelector('[data-target="tab-huggingface"]').click();
            window.showToast("Check out the curated models to get started!", "info");
        });
    }
};
