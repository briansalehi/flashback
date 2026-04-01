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
    
    formatISODate(date) {
        if (!date) return '';
        const d = date instanceof Date ? date : new Date(date);
        return d.toISOString().split('T')[0];
    },
    
    getLocalISODate(date = new Date()) {
        const d = date instanceof Date ? date : new Date(date);
        const year = d.getFullYear();
        const month = String(d.getMonth() + 1).padStart(2, '0');
        const day = String(d.getDate()).padStart(2, '0');
        return `${year}-${month}-${day}`;
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
    },
    
    capitalizeWords(str) {
        if (!str) return '';
        return str.trim()
            .split(/\s+/)
            .map(word => word.charAt(0).toUpperCase() + word.slice(1).toLowerCase())
            .join(' ');
    },

    showVerificationModal() {
        if (document.getElementById('verification-modal')) {
            return;
        }

        const modalOverlay = document.createElement('div');
        modalOverlay.id = 'verification-modal';
        modalOverlay.className = 'modal-overlay';
        modalOverlay.style.display = 'flex';

        modalOverlay.innerHTML = `
            <div class="modal-content">
                <div class="modal-header">
                    <h2 class="modal-title">Verification Required</h2>
                </div>
                <div class="modal-body">
                    <p style="margin-bottom: 1.5rem; color: var(--color-text-secondary);">
                        You need to verify your email address before you can make any changes.
                        Please check your inbox or go to the account page to send a new verification code.
                    </p>
                </div>
                <div class="modal-actions">
                    <button id="go-to-account-btn" class="btn btn-primary" style="padding: 0.5rem 1.5rem; white-space: nowrap;">
                        Go to Account
                    </button>
                    <button id="close-verification-modal-btn" class="btn btn-secondary" style="padding: 0.5rem 1.5rem; white-space: nowrap;">
                        Dismiss
                    </button>
                </div>
            </div>
        `;

        document.body.appendChild(modalOverlay);

        document.getElementById('go-to-account-btn').addEventListener('click', () => {
            window.location.href = '/account.html';
        });

        document.getElementById('close-verification-modal-btn').addEventListener('click', () => {
            document.body.removeChild(modalOverlay);
        });
    },

    /**
     * Render breadcrumbs in a standardized way
     * @param {Array<{name: string, url: string}>} items - List of breadcrumb items
     * @param {string} containerId - ID of the container element (default: 'breadcrumb')
     */
    renderBreadcrumbs(items, containerId = 'breadcrumb') {
        const container = document.getElementById(containerId);
        if (!container) return;

        if (!items || items.length === 0) {
            container.innerHTML = '';
            container.style.display = 'none';
            return;
        }

        container.innerHTML = '';
        container.className = 'breadcrumb-container';
        container.style.display = 'flex';

        const breadcrumbList = document.createElement('nav');
        breadcrumbList.className = 'breadcrumb-nav';
        breadcrumbList.setAttribute('aria-label', 'Breadcrumb');

        const list = document.createElement('ol');
        list.className = 'breadcrumb-list';

        items.forEach((item, index) => {
            const itemEl = document.createElement('li');
            const isLast = index === items.length - 1;
            itemEl.className = 'breadcrumb-item' + (isLast ? ' is-active' : '');
            
            const link = document.createElement('a');
            link.href = item.url;
            link.textContent = item.name;
            
            itemEl.appendChild(link);
            list.appendChild(itemEl);
        });

        breadcrumbList.appendChild(list);
        container.appendChild(breadcrumbList);
    }
};
