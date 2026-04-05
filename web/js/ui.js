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
            if (!show && document.activeElement && el.contains(document.activeElement)) {
                document.activeElement.blur();
            }
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
     * Keyboard Shortcuts Manager
     */
    Shortcuts: {
        shortcuts: {},
        isEnabled() {
            // Shortcuts are disabled by default
            const setting = localStorage.getItem('flashback_shortcuts_enabled');
            const mobileView = window.innerWidth <= 768;
            return setting === 'true' && !mobileView;
        },
        register(key, description, action) {
            this.shortcuts[key] = { description, action };
        },
        scrollToTop() {
            const visibleOverlays = Array.from(document.querySelectorAll('.modal-overlay')).filter(m => m.style.display !== 'none');
            if (visibleOverlays.length > 0) {
                const modalBody = visibleOverlays[visibleOverlays.length - 1].querySelector('.modal-body, .modal-content');
                if (modalBody) modalBody.scrollTo({ top: 0, behavior: 'smooth' });
            } else {
                window.scrollTo({ top: 0, behavior: 'smooth' });
            }
        },
        scrollToBottom() {
            const visibleOverlays = Array.from(document.querySelectorAll('.modal-overlay')).filter(m => m.style.display !== 'none');
            if (visibleOverlays.length > 0) {
                const modalBody = visibleOverlays[visibleOverlays.length - 1].querySelector('.modal-body, .modal-content');
                if (modalBody) modalBody.scrollTo({ top: modalBody.scrollHeight, behavior: 'smooth' });
            } else {
                window.scrollTo({ top: document.documentElement.scrollHeight, behavior: 'smooth' });
            }
        },
        init() {
            this.lastKeyPress = null;
            this.lastKeyPressTime = 0;

            document.addEventListener('keydown', (e) => {
                // Esc key should cancel any modal and key operation
                if (e.key === 'Escape') {
                    // Find the topmost visible modal
                    const visibleOverlays = Array.from(document.querySelectorAll('.modal-overlay')).filter(m => m.style.display !== 'none');
                    if (visibleOverlays.length > 0) {
                        const topmostOverlay = visibleOverlays[visibleOverlays.length - 1];
                        
                        // Prefer visible "Back" buttons in step-based modals
                        const backBtn = topmostOverlay.querySelector('#back-to-cards-btn, .btn-back');
                        if (backBtn && backBtn.style.display !== 'none' && backBtn.offsetHeight > 0) {
                            backBtn.click();
                            return;
                        }

                        // Otherwise find close or cancel button
                        const closeBtns = Array.from(topmostOverlay.querySelectorAll('.modal-close, [id^="cancel-"], [id^="close-"], .btn-secondary'));
                        const closeBtn = closeBtns.find(btn => btn.style.display !== 'none' && btn.offsetHeight > 0);

                        if (closeBtn) {
                            closeBtn.click();
                        } else {
                            topmostOverlay.style.display = 'none';
                        }
                    } else {
                        // Trigger custom event for other scripts to handle Escape when no modal is open
                        window.dispatchEvent(new CustomEvent('escapePressed'));
                    }
                    return;
                }

                // Ctrl+Enter for Save in Edit/Add modals
                if (e.ctrlKey && e.key === 'Enter') {
                    const visibleOverlays = Array.from(document.querySelectorAll('.modal-overlay')).filter(m => m.style.display !== 'none');
                    if (visibleOverlays.length > 0) {
                        const activeModal = visibleOverlays[visibleOverlays.length - 1];
                        // Find a primary/save button
                        const primaryBtn = activeModal.querySelector('.btn-primary, #save-edit-block-modal-btn, #save-block-btn, #save-edit-card-btn');
                        if (primaryBtn) {
                            e.preventDefault();
                            primaryBtn.click();
                            return;
                        }
                    }
                }

                // Enter handler (only if no modifiers)
                if (e.key === 'Enter' && !e.ctrlKey && !e.shiftKey && !e.altKey) {
                    // If a button is focused, let the browser handle it (standard behavior)
                    if (e.target.tagName === 'BUTTON') {
                        return;
                    }

                    const visibleOverlays = Array.from(document.querySelectorAll('.modal-overlay')).filter(m => m.style.display !== 'none');
                    if (visibleOverlays.length > 0) {
                        const activeModal = visibleOverlays[visibleOverlays.length - 1];
                        
                        // If we are in a textarea, Enter should be allowed (standard behavior)
                        if (e.target.tagName === 'TEXTAREA') {
                            return;
                        }

                        // For confirmation/remove/review/move modals, we auto-click the confirm button if no button is focused
                        if (activeModal.id.includes('confirm') || activeModal.id.includes('remove') || activeModal.id.includes('review') || activeModal.id.includes('move')) {
                            const confirmBtn = activeModal.querySelector('#confirm-modal-btn, #confirm-remove-block-btn, #confirm-remove-card-btn, #confirm-review-btn, #confirm-move-block-btn, .btn-primary, .btn-danger, .headline-option.highlighted, .target-block-gap.highlighted');
                            
                            if (confirmBtn) {
                                e.preventDefault();
                                confirmBtn.click();
                                return;
                            }
                        } else {
                            // For other modals (like Edit), Enter alone should NOT trigger save (forcing Ctrl+Enter or focused button)
                            // We do nothing here, allowing standard behavior (which might be nothing if it's just an input)
                        }
                    }
                }

                if (!this.isEnabled()) return;
                
                // Don't trigger if user is typing in an input or textarea (unless Alt is pressed)
                if ((e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') && !e.altKey) return;

                // Make sure these shortcuts won't trigger when pressed with Ctrl or Alt keys
                // EXCEPT Alt key is allowed for some shortcuts (like Alt + Number)
                // Shift is allowed for single characters (e.g., 'G')
                if (e.ctrlKey || (e.shiftKey && e.key.length > 1)) return;
                
                // If Alt is pressed, we only allow it for specific shortcuts (e.g. breadcrumbs)
                if (e.altKey && !/[0-9]/.test(e.key.toLowerCase())) return;
                
                // If NOT Alt is pressed, but it's a number, it's for block selection
                if (!e.altKey && /[0-9]/.test(e.key)) {
                    // This will fall through to shortcut handling
                }

                // Handle gg shortcut
                if (e.key === 'g' && !e.altKey && !e.ctrlKey && !e.shiftKey) {
                    const now = Date.now();
                    if (this.lastKeyPress === 'g' && (now - this.lastKeyPressTime) < 500) {
                        this.lastKeyPress = null;
                        this.lastKeyPressTime = 0;
                        if (this.shortcuts['gg']) {
                            e.preventDefault();
                            this.shortcuts['gg'].action();
                            return;
                        }
                    }
                    this.lastKeyPress = 'g';
                    this.lastKeyPressTime = now;
                    // Note: don't return here, 'g' might be a single-key shortcut
                } else {
                    this.lastKeyPress = e.key;
                    this.lastKeyPressTime = Date.now();
                }

                const key = (e.altKey ? 'alt+' : '') + (e.key.length === 1 ? e.key : e.key.toLowerCase());
                if (key === '?') {
                    this.showHelp();
                    return;
                }

                if (this.shortcuts[key]) {
                    e.preventDefault();
                    this.shortcuts[key].action();
                } else if (this.shortcuts[key.toLowerCase()]) {
                    // Fallback to lowercase for convenience (e.g. Shift+A works for 'a')
                    // EXCEPT for 'G' which we specifically want to handle separately
                    if (key !== 'G') {
                        e.preventDefault();
                        this.shortcuts[key.toLowerCase()].action();
                    }
                }
            });

            // Global scrolling shortcuts
            this.register('j', 'Scroll Down', () => {
                const visibleOverlays = Array.from(document.querySelectorAll('.modal-overlay')).filter(m => m.style.display !== 'none');
                if (visibleOverlays.length > 0) {
                    const modalBody = visibleOverlays[visibleOverlays.length - 1].querySelector('.modal-body, .modal-content');
                    if (modalBody) modalBody.scrollBy({ top: 100, behavior: 'smooth' });
                } else {
                    window.scrollBy({ top: 100, behavior: 'smooth' });
                }
            });
            this.register('k', 'Scroll Up', () => {
                const visibleOverlays = Array.from(document.querySelectorAll('.modal-overlay')).filter(m => m.style.display !== 'none');
                if (visibleOverlays.length > 0) {
                    const modalBody = visibleOverlays[visibleOverlays.length - 1].querySelector('.modal-body, .modal-content');
                    if (modalBody) modalBody.scrollBy({ top: -100, behavior: 'smooth' });
                } else {
                    window.scrollBy({ top: -100, behavior: 'smooth' });
                }
            });
            this.register('d', 'Jump Down', () => {
                const visibleOverlays = Array.from(document.querySelectorAll('.modal-overlay')).filter(m => m.style.display !== 'none');
                if (visibleOverlays.length > 0) {
                    const modalBody = visibleOverlays[visibleOverlays.length - 1].querySelector('.modal-body, .modal-content');
                    if (modalBody) modalBody.scrollBy({ top: 300, behavior: 'smooth' });
                } else {
                    window.scrollBy({ top: 300, behavior: 'smooth' });
                }
            });
            this.register('u', 'Jump Up', () => {
                const visibleOverlays = Array.from(document.querySelectorAll('.modal-overlay')).filter(m => m.style.display !== 'none');
                if (visibleOverlays.length > 0) {
                    const modalBody = visibleOverlays[visibleOverlays.length - 1].querySelector('.modal-body, .modal-content');
                    if (modalBody) modalBody.scrollBy({ top: -300, behavior: 'smooth' });
                } else {
                    window.scrollBy({ top: -300, behavior: 'smooth' });
                }
            });
            this.register('gg', 'Go to Top', () => this.scrollToTop());
            this.register('G', 'Go to Bottom', () => this.scrollToBottom());
        },
        showHelp() {
            if (document.getElementById('shortcuts-help-modal')) {
                UI.toggleElement('shortcuts-help-modal', true);
                return;
            }

            const modalOverlay = document.createElement('div');
            modalOverlay.id = 'shortcuts-help-modal';
            modalOverlay.className = 'modal-overlay';
            modalOverlay.style.display = 'flex';

            let shortcutsList = '';
            for (const [key, data] of Object.entries(this.shortcuts)) {
                shortcutsList += `
                    <div style="display: flex; justify-content: space-between; padding: var(--space-xs) 0; border-bottom: 1px solid var(--border-color);">
                        <span style="font-family: 'JetBrains Mono', monospace; color: var(--color-primary-start); font-weight: bold;">[${key}]</span>
                        <span style="color: var(--color-text-secondary);">${data.description}</span>
                    </div>
                `;
            }

            modalOverlay.innerHTML = `
                <div class="modal-content" style="max-width: 400px;">
                    <div class="modal-header">
                        <h2 class="modal-title">Keyboard Shortcuts</h2>
                        <button id="close-shortcuts-help-btn" class="modal-close">&times;</button>
                    </div>
                    <div class="modal-body">
                        ${shortcutsList}
                        <div style="display: flex; justify-content: space-between; padding: var(--space-xs) 0; border-bottom: 1px solid var(--border-color);">
                            <span style="font-family: 'JetBrains Mono', monospace; color: var(--color-primary-start); font-weight: bold;">[?]</span>
                            <span style="color: var(--color-text-secondary);">Show this help</span>
                        </div>
                        <div style="display: flex; justify-content: space-between; padding: var(--space-xs) 0; border-bottom: 1px solid var(--border-color);">
                            <span style="font-family: 'JetBrains Mono', monospace; color: var(--color-primary-start); font-weight: bold;">[Esc]</span>
                            <span style="color: var(--color-text-secondary);">Cancel / Close</span>
                        </div>
                    </div>
                </div>
            `;

            document.body.appendChild(modalOverlay);
            document.getElementById('close-shortcuts-help-btn').onclick = () => {
                UI.toggleElement('shortcuts-help-modal', false);
            };
            
            // Close on overlay click
            modalOverlay.onclick = (e) => {
                if (e.target === modalOverlay) UI.toggleElement('shortcuts-help-modal', false);
            };
        }
    },

    autoInitShortcuts() {
        this.Shortcuts.init();
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
        return `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="resource-icon"><path d="M7 22v-1a2 2 0 0 1 2-2h6a2 2 0 0 0 2-2v-1a2 2 0 0 0-2-2H9a2 2 0 0 1-2-2v-1a2 2 0 0 1 2-2h6a2 2 0 0 0 2-2v-1a2 2 0 0 0-2-2H9a2 2 0 0 1-2-2V2"></path><circle cx="7" cy="22" r="1" fill="currentColor"></circle><circle cx="15" cy="19" r="1" fill="currentColor"></circle><circle cx="9" cy="9" r="1" fill="currentColor"></circle><circle cx="7" cy="2" r="1" fill="currentColor"></circle></svg>`;
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
        container.style.display = 'block';

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

document.addEventListener('DOMContentLoaded', () => {
    if (typeof UI !== 'undefined' && UI.autoInitShortcuts) {
        UI.autoInitShortcuts();
    }
});
