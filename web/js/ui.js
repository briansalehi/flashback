/**
 * UI Helper Functions
 */

const UI = {
    /**
     * Show error message
     */
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
    
    /**
     * Show success message
     */
    showSuccess(message, elementId = 'success-message') {
        const successEl = document.getElementById(elementId);
        if (successEl) {
            successEl.textContent = message;
            successEl.style.display = 'block';
            successEl.className = 'alert alert-success';
        }
    },
    
    /**
     * Hide message
     */
    hideMessage(elementId) {
        const el = document.getElementById(elementId);
        if (el) {
            el.style.display = 'none';
        }
    },
    
    /**
     * Show loading state on button
     */
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
    
    /**
     * Format date
     */
    formatDate(date) {
        if (!date) return '';
        const d = date instanceof Date ? date : new Date(date);
        return d.toLocaleDateString('en-US', { 
            year: 'numeric', 
            month: 'long', 
            day: 'numeric' 
        });
    },
    
    /**
     * Calculate progress percentage
     */
    calculateProgress(completed, total) {
        if (total === 0) return 0;
        return Math.round((completed / total) * 100);
    },
    
    /**
     * Render roadmap card
     */
    renderRoadmapCard(roadmap, completedCount = 0, totalCount = 0) {
        const progress = this.calculateProgress(completedCount, totalCount);
        
        return `
            <a href="roadmap.html?id=${roadmap.id}" class="roadmap-card">
                <h3 class="roadmap-title">${this.escapeHtml(roadmap.name)}</h3>
                <p class="roadmap-description">${this.escapeHtml(roadmap.description || 'No description')}</p>
                <div class="roadmap-meta">
                    <span>${completedCount} of ${totalCount} milestones</span>
                    <span>${progress}%</span>
                </div>
                <div class="roadmap-progress">
                    <div class="roadmap-progress-bar" style="width: ${progress}%"></div>
                </div>
            </a>
        `;
    },
    
    /**
     * Render milestone item
     */
    renderMilestone(milestone) {
        const completedClass = milestone.completed ? 'completed' : '';
        const targetDate = milestone.targetDate ? 
            `<span class="text-muted">Due: ${this.formatDate(milestone.targetDate)}</span>` : '';
        
        return `
            <div class="timeline-item ${completedClass}">
                <div class="card">
                    <h4>${this.escapeHtml(milestone.name)}</h4>
                    <p class="text-muted">${this.escapeHtml(milestone.description || '')}</p>
                    ${targetDate}
                    <a href="subject.html?milestone=${milestone.id}" class="btn btn-secondary mt-md">View Subjects</a>
                </div>
            </div>
        `;
    },
    
    /**
     * Escape HTML to prevent XSS
     */
    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    },
    
    /**
     * Get URL parameter
     */
    getUrlParam(param) {
        const urlParams = new URLSearchParams(window.location.search);
        return urlParams.get(param);
    },
    
    /**
     * Show/hide element
     */
    toggleElement(elementId, show) {
        const el = document.getElementById(elementId);
        if (el) {
            el.style.display = show ? 'block' : 'none';
        }
    },
    
    /**
     * Clear form
     */
    clearForm(formId) {
        const form = document.getElementById(formId);
        if (form) {
            form.reset();
        }
    }
};
