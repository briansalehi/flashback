const UI = {
    showError(message, elementId = 'error-message') {
        const errorEl = document.getElementById(elementId);
        if (errorEl) {
            errorEl.textContent = message;
            errorEl.style.display = 'block';
            errorEl.className = 'alert alert-error';
        } else {
            alert('Error: ' + message);
        }
    },
    
    showSuccess(message, elementId = 'success-message') {
        const successEl = document.getElementById(elementId);
        if (successEl) {
            successEl.textContent = message;
            successEl.style.display = 'block';
            successEl.className = 'alert alert-success';
        }
    },
    
    hideMessage(elementId) {
        const el = document.getElementById(elementId);
        if (el) {
            el.style.display = 'none';
        }
    },
    
    setButtonLoading(buttonId, loading = true) {
        const btn = document.getElementById(buttonId);
        if (!btn) return;
        
        if (loading) {
            btn.disabled = true;
            btn.dataset.originalText = btn.innerHTML;
            btn.innerHTML = '<span class="loading"></span> Loading...';
        } else {
            btn.disabled = false;
            btn.innerHTML = btn.dataset.originalText || btn.innerHTML;
        }
    },
    
    formatDate(date) {
        if (!date) return '';
        const d = date instanceof Date ? date : new Date(date);
        return d.toLocaleDateString('en-US', { 
            year: 'numeric', 
            month: 'long', 
            day: 'numeric' 
        });
    },
    
    calculateProgress(completed, total) {
        if (total === 0) return 0;
        return Math.round((completed / total) * 100);
    },
    
    renderRoadmap(roadmap) {
    },
    
    /**
     * Escape HTML to prevent XSS
     */
    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    },
    
    getUrlParam(param) {
        const urlParams = new URLSearchParams(window.location.search);
        return urlParams.get(param);
    },
    
    toggleElement(elementId, show) {
        const el = document.getElementById(elementId);
        if (el) {
            el.style.display = show ? 'block' : 'none';
        }
    },
    
    clearForm(formId) {
        const form = document.getElementById(formId);
        if (form) {
            form.reset();
        }
    }
};
