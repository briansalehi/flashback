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

    /**
     * Get icon for a resource type
     * @param {number} type - Resource type ID
     * @returns {string} SVG icon string
     */
    getResourceIcon(type) {
        const icons = {
            0: `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"></path><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"></path></svg>`, // book
            1: `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><line x1="2" y1="12" x2="22" y2="12"></line><path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"></path></svg>`, // website
            2: `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M22 10v6M2 10l10-5 10 5-10 5z"></path><path d="M6 12v5c3 3 9 3 12 0v-5"></path></svg>`, // course
            3: `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="2" width="20" height="20" rx="2.18" ry="2.18"></rect><line x1="7" y1="2" x2="7" y2="22"></line><line x1="17" y1="2" x2="17" y2="22"></line><line x1="2" y1="12" x2="22" y2="12"></line><line x1="2" y1="7" x2="7" y2="7"></line><line x1="2" y1="17" x2="7" y2="17"></line><line x1="17" y1="17" x2="22" y2="17"></line><line x1="17" y1="7" x2="22" y2="7"></line></svg>`, // video (film strip style)
            4: `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M22.54 6.42a2.78 2.78 0 0 0-1.94-2C18.88 4 12 4 12 4s-6.88 0-8.6.46a2.78 2.78 0 0 0-1.94 2A29 29 0 0 0 1 11.75a29 29 0 0 0 .46 5.33A2.78 2.78 0 0 0 3.4 19c1.72.46 8.6.46 8.6.46s6.88 0 8.6-.46a2.78 2.78 0 0 0 1.94-2 29 29 0 0 0 .46-5.25 29 29 0 0 0-.46-5.33z"></path><polygon points="9.75 15.02 15.5 11.75 9.75 8.48 9.75 15.02"></polygon></svg>`, // channel (YouTube style)
            5: `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 4h16c1.1 0 2 .9 2 2v12c0 1.1-.9 2-2 2H4c-1.1 0-2-.9-2-2V6c0-1.1.9-2 2-2z"></path><polyline points="22,6 12,13 2,6"></polyline></svg>`, // mailing_list
            6: `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path><polyline points="14 2 14 8 20 8"></polyline><line x1="16" y1="13" x2="8" y2="13"></line><line x1="16" y1="17" x2="8" y2="17"></line><polyline points="10 9 9 9 8 9"></polyline></svg>`, // manual
            7: `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="3" width="20" height="14" rx="2" ry="2"></rect><path d="M8 21h8"></path><path d="M12 17v4"></path><path d="M7 8h10"></path><path d="M7 12h7"></path></svg>`, // slides (presentation screen style)
            8: `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M15 14c.2-1 .7-1.7 1.5-2.5 1-.9 1.5-2.2 1.5-3.5A5 5 0 0 0 8 8c0 1.3.5 2.6 1.5 3.5.8.8 1.3 1.5 1.5 2.5"></path><path d="M9 18h6"></path><path d="M10 22h4"></path></svg>`, // knowledge (lightbulb)
        };
        const fallback = `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M13 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z"></path><polyline points="13 2 13 9 20 9"></polyline></svg>`;
        return icons[type] || fallback;
    },

    getRoadmapIcon() {
        return `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="1 6 1 22 8 18 16 22 23 18 23 2 16 6 8 2 1 6"></polygon><line x1="8" y1="2" x2="8" y2="18"></line><line x1="16" y1="6" x2="16" y2="22"></line></svg>`;
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
            
            let content;
            if (item.url) {
                content = document.createElement('a');
                content.href = item.url;
            } else {
                content = document.createElement('span');
                content.className = 'breadcrumb-text';
            }
            
            if (item.icon) {
                const iconSpan = document.createElement('span');
                iconSpan.className = 'resource-icon';
                iconSpan.innerHTML = item.icon;
                iconSpan.style.marginRight = 'var(--space-xs)';
                content.appendChild(iconSpan);
            }
            
            const textSpan = document.createElement('span');
            textSpan.textContent = item.name;
            content.appendChild(textSpan);
            
            itemEl.appendChild(content);
            list.appendChild(itemEl);
        });

        breadcrumbList.appendChild(list);
        container.appendChild(breadcrumbList);
    }
};
