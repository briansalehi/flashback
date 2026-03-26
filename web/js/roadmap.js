let currentMilestones = [];
let reorderState = {
    active: false,
    sourceIndex: null,
    longPressTimer: null,
    preventClick: false,
    startPos: { x: 0, y: 0 }
};

function enterReorderMode(index) {
    if (reorderState.active) return;
    
    reorderState.active = true;
    reorderState.sourceIndex = index;
    reorderState.preventClick = true;
    
    const container = document.getElementById('milestones-container');
    if (container) {
        container.classList.add('reorder-mode-active');
    }
    
    // Highlight source
    const sourceCard = container.children[index];
    if (sourceCard) {
        sourceCard.classList.add('reorder-source');
    }
    
    // Add hint at the top of the list
    const hint = document.createElement('div');
    hint.id = 'reorder-hint';
    hint.className = 'reorder-hint';
    hint.innerHTML = `
        <span style="font-weight: 600; font-size: 0.95rem;">Select target location to move this milestone</span>
        <button class="btn btn-secondary btn-sm" onclick="exitReorderMode()" style="padding: 4px 12px; font-size: 0.85rem; background: rgba(255,255,255,0.2); border: none;">Cancel</button>
    `;
    document.body.appendChild(hint);
    
    if (navigator.vibrate) navigator.vibrate(50);
}

window.exitReorderMode = function() {
    reorderState.active = false;
    reorderState.sourceIndex = null;
    
    const container = document.getElementById('milestones-container');
    if (container) {
        container.classList.remove('reorder-mode-active');
        document.querySelectorAll('.item-block').forEach(b => {
            b.classList.remove('reorder-source');
        });
    }
    
    const hint = document.getElementById('reorder-hint');
    if (hint) hint.remove();
};

// Confirmation Modal Functions
(function() {
    const confirmModal = document.getElementById('confirm-modal');
    if (!confirmModal) return;

    const closeConfirmModal = () => {
        confirmModal.style.display = 'none';
        document.body.style.overflow = '';
        UI.setButtonLoading('confirm-modal-btn', false);
    };

    const closeBtn = document.getElementById('close-confirm-modal-btn');
    if (closeBtn) closeBtn.addEventListener('click', closeConfirmModal);

    const cancelBtn = document.getElementById('cancel-confirm-modal-btn');
    if (cancelBtn) cancelBtn.addEventListener('click', closeConfirmModal);

    const confirmBtn = document.getElementById('confirm-modal-btn');
    if (confirmBtn) {
        confirmBtn.addEventListener('click', async () => {
            if (typeof confirmModal._confirmCallback === 'function') {
                UI.setButtonLoading('confirm-modal-btn', true);
                try {
                    await confirmModal._confirmCallback();
                } finally {
                    UI.setButtonLoading('confirm-modal-btn', false);
                    closeConfirmModal();
                }
            } else {
                closeConfirmModal();
            }
        });
    }

    if (confirmModal) {
        confirmModal.addEventListener('click', (e) => {
            if (e.target === confirmModal) closeConfirmModal();
        });
    }

    window.showConfirmModal = (title, message, callback) => {
        const titleEl = document.getElementById('confirm-modal-title');
        const messageEl = document.getElementById('confirm-modal-message');
        if (titleEl) titleEl.textContent = title;
        if (messageEl) messageEl.textContent = message;
        confirmModal._confirmCallback = callback;
        confirmModal.style.display = 'flex';
        document.body.style.overflow = 'hidden';
    };
})();

window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    const roadmapId = UI.getUrlParam('id');
    const roadmapName = UI.getUrlParam('name');
    if (!roadmapId) {
        window.location.href = '/home.html';
        return;
    }

    // Set roadmap title
    const roadmapTitle = document.getElementById('roadmap-name');
    if (roadmapTitle) {
        roadmapTitle.textContent = roadmapName || 'Roadmap';
        // Buttons are now always visible, no toggle needed
    }

    const signoutBtn = document.getElementById('signout-btn');
    if (signoutBtn) {
        signoutBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            await client.signOut();
            localStorage.removeItem('token');
            window.location.href = '/index.html';
        });
    }

    const createMilestoneBtn = document.getElementById('create-milestone-btn');
    if (createMilestoneBtn) {
        createMilestoneBtn.addEventListener('click', (e) => {
            e.preventDefault();

            // Show the form
            UI.toggleElement('add-milestone-form-overlay', true);
            document.body.style.overflow = 'hidden'; // Prevent scrolling background
            
            // Initial state: search mode
            UI.toggleElement('search-subject-section', true);
            UI.toggleElement('create-subject-section', false);
            UI.toggleElement('milestone-addition-group', true);
            UI.toggleElement('create-mode-actions', false);
            
            // Use setTimeout to ensure the element is visible before focusing
            setTimeout(() => {
                const searchInput = document.getElementById('subject-search-input');
                if (searchInput) {
                    searchInput.disabled = false;
                    searchInput.focus();
                }
            }, 100);
        });
    }

    const closeAddMilestoneModalBtn = document.getElementById('close-add-milestone-modal-btn');
    const cancelMilestoneBtn = document.getElementById('cancel-milestone-btn');
    const addMilestoneOverlay = document.getElementById('add-milestone-form-overlay');

    const closeAddMilestoneModal = () => {
        UI.toggleElement('add-milestone-form-overlay', false);
        document.body.style.overflow = ''; // Restore scrolling
        UI.clearForm('milestone-form');
        clearSearchResults();
    };
    
    if (closeAddMilestoneModalBtn) closeAddMilestoneModalBtn.addEventListener('click', closeAddMilestoneModal);
    if (cancelMilestoneBtn) cancelMilestoneBtn.addEventListener('click', closeAddMilestoneModal);

    if (addMilestoneOverlay) {
        addMilestoneOverlay.addEventListener('click', (e) => {
            if (e.target === addMilestoneOverlay) {
                closeAddMilestoneModal();
            }
        });
    }

    const milestoneSearchInput = document.getElementById('milestone-search-input');
    if (milestoneSearchInput) {
        milestoneSearchInput.addEventListener('input', (e) => {
            const searchTerm = e.target.value.toLowerCase().trim();
            filterMilestones(searchTerm);
        });
    }

    const createNewSubjectBtn = document.getElementById('create-new-subject-btn');
    if (createNewSubjectBtn) {
        createNewSubjectBtn.addEventListener('click', () => {
            // Enter Create Mode
            UI.toggleElement('create-subject-section', true);
            UI.toggleElement('search-subject-section', false);
            UI.toggleElement('milestone-addition-group', false);
            UI.toggleElement('create-mode-actions', true);
            
            setTimeout(() => {
                const nameInput = document.getElementById('new-subject-name');
                if (nameInput) {
                    nameInput.focus();
                }
            }, 100);
        });
    }

    const backToSearchBtn = document.getElementById('back-to-search-btn');
    if (backToSearchBtn) {
        backToSearchBtn.addEventListener('click', () => {
            // Return to Search Mode
            UI.toggleElement('create-subject-section', false);
            UI.toggleElement('search-subject-section', true);
            UI.toggleElement('milestone-addition-group', true);
            UI.toggleElement('create-mode-actions', false);
            UI.clearForm('create-subject-form');
        });
    }

    let searchTimeout;
    const subjectSearchInput = document.getElementById('subject-search-input');
    if (subjectSearchInput) {
        subjectSearchInput.addEventListener('input', (e) => {
            clearTimeout(searchTimeout);
            const searchToken = e.target.value.trim();

            searchTimeout = setTimeout(async () => {
                await searchSubjects(searchToken);
            }, 300);
        });
    } else {
        console.error('Search input not found!');
    }

    const saveNewSubjectBtn = document.getElementById('save-new-subject-btn');
    if (saveNewSubjectBtn) {
        saveNewSubjectBtn.addEventListener('click', async () => {
            const nameInput = document.getElementById('new-subject-name');
            const name = nameInput.value.trim();

            if (!name) {
                UI.showError('Please enter a subject name');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-new-subject-btn', true);

            try {
                await client.createSubject(name);

                // After creating, search for it to add to roadmap
                await searchSubjects(name);

                // Exit Create Mode and return to Search Mode automatically
                UI.toggleElement('create-subject-section', false);
                UI.toggleElement('search-subject-section', true);
                UI.toggleElement('milestone-addition-group', true);
                UI.toggleElement('create-mode-actions', false);
                
                nameInput.value = '';
                UI.setButtonLoading('save-new-subject-btn', false);

                // Focus on the first search result (which should be the new subject)
                setTimeout(() => {
                    const firstResult = document.querySelector('input[name="subject-select"]');
                    if (firstResult) {
                        firstResult.checked = true;
                        firstResult.focus();
                        // Trigger change event to highlight the selected item
                        firstResult.dispatchEvent(new Event('change', { bubbles: true }));
                    }
                }, 100);

                UI.showSuccess('Subject created and selected! Click "Add to Roadmap" below.');
            } catch (err) {
                console.error('Create subject failed:', err);
                UI.showError(err.message || 'Failed to create subject');
                UI.setButtonLoading('save-new-subject-btn', false);
            }
        });
    }

    const milestoneForm = document.getElementById('milestone-form');
    if (milestoneForm) {
        milestoneForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const selectedSubject = document.querySelector('input[name="subject-select"]:checked');
            if (!selectedSubject) {
                UI.showError('Please select a subject');
                return;
            }

            const subjectId = selectedSubject.value;
            const level = document.getElementById('expertise-level').value;

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-milestone-btn', true);

            try {
                await client.addMilestone(roadmapId, subjectId, parseInt(level));

                UI.toggleElement('add-milestone-form-overlay', false);
                document.body.style.overflow = ''; // Restore scrolling
                UI.clearForm('milestone-form');
                clearSearchResults();
                UI.setButtonLoading('save-milestone-btn', false);

                loadMilestones();
            } catch (err) {
                console.error('Add milestone failed:', err);
                UI.showError(err.message || 'Failed to add subject to roadmap');
                UI.setButtonLoading('save-milestone-btn', false);
            }
        });
    }

    // Rename roadmap handlers
    const renameRoadmapBtn = document.getElementById('rename-roadmap-btn');
    const renameModal = document.getElementById('rename-roadmap-modal');
    const closeRenameModalBtn = document.getElementById('close-rename-modal-btn');
    const cancelRenameBtn = document.getElementById('cancel-rename-btn');

    if (renameRoadmapBtn) {
        renameRoadmapBtn.addEventListener('click', (e) => {
            e.preventDefault();
            UI.toggleElement('rename-roadmap-modal', true);
            document.body.style.overflow = 'hidden';
            const roadmapNameCurrent = document.getElementById('roadmap-name').textContent;
            document.getElementById('rename-roadmap-name').value = roadmapNameCurrent || roadmapName || '';
            setTimeout(() => {
                document.getElementById('rename-roadmap-name').focus();
            }, 100);
        });
    }

    const closeRenameModal = () => {
        UI.toggleElement('rename-roadmap-modal', false);
        document.body.style.overflow = '';
        UI.clearForm('rename-roadmap-form');
    };

    if (closeRenameModalBtn) closeRenameModalBtn.addEventListener('click', closeRenameModal);
    if (cancelRenameBtn) cancelRenameBtn.addEventListener('click', closeRenameModal);
    if (renameModal) {
        renameModal.addEventListener('click', (e) => {
            if (e.target === renameModal) closeRenameModal();
        });
    }

    const renameRoadmapForm = document.getElementById('rename-roadmap-form');
    if (renameRoadmapForm) {
        renameRoadmapForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const newNameInput = document.getElementById('rename-roadmap-name');
            const newName = UI.capitalizeWords(newNameInput.value);
            if (!newName) {
                UI.showError('Please enter a roadmap name');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-rename-btn', true);

            try {
                await client.renameRoadmap(roadmapId, newName);

                closeRenameModal();
                UI.setButtonLoading('save-rename-btn', false);

                // Update the URL and page
                const newUrl = `roadmap.html?id=${roadmapId}&name=${encodeURIComponent(newName)}`;
                window.history.replaceState({}, '', newUrl);
                document.getElementById('roadmap-name').textContent = newName;
                document.title = `${newName} - Flashback`;

                UI.showSuccess('Roadmap renamed successfully');
            } catch (err) {
                console.error('Rename roadmap failed:', err);
                UI.showError(err.message || 'Failed to rename roadmap');
                UI.setButtonLoading('save-rename-btn', false);
            }
        });
    }

    // Remove roadmap handlers
    const removeRoadmapBtn = document.getElementById('remove-roadmap-btn');
    const removeRoadmapModal = document.getElementById('remove-roadmap-modal');
    const closeRemoveRoadmapModalBtn = document.getElementById('close-remove-roadmap-modal-btn');
    const cancelRemoveBtn = document.getElementById('cancel-remove-btn');

    if (removeRoadmapBtn) {
        removeRoadmapBtn.addEventListener('click', (e) => {
            e.preventDefault();
            UI.toggleElement('remove-roadmap-modal', true);
            document.body.style.overflow = 'hidden';
        });
    }

    const closeRemoveRoadmapModal = () => {
        UI.toggleElement('remove-roadmap-modal', false);
        document.body.style.overflow = '';
    };

    if (closeRemoveRoadmapModalBtn) closeRemoveRoadmapModalBtn.addEventListener('click', closeRemoveRoadmapModal);
    if (cancelRemoveBtn) cancelRemoveBtn.addEventListener('click', closeRemoveRoadmapModal);
    if (removeRoadmapModal) {
        removeRoadmapModal.addEventListener('click', (e) => {
            if (e.target === removeRoadmapModal) closeRemoveRoadmapModal();
        });
    }

    const confirmRemoveBtn = document.getElementById('confirm-remove-btn');
    if (confirmRemoveBtn) {
        confirmRemoveBtn.addEventListener('click', async () => {
            UI.hideMessage('error-message');
            UI.setButtonLoading('confirm-remove-btn', true);

            try {
                await client.removeRoadmap(roadmapId);

                closeRemoveRoadmapModal();
                UI.setButtonLoading('confirm-remove-btn', false);

                // Redirect to home page
                window.location.href = '/home.html';
            } catch (err) {
                console.error('Remove roadmap failed:', err);
                UI.showError(err.message || 'Failed to remove roadmap');
                UI.setButtonLoading('confirm-remove-btn', false);
            }
        });
    }

    // Remove milestone handlers
    const removeMilestoneModal = document.getElementById('remove-milestone-modal');
    const closeRemoveMilestoneModalBtn = document.getElementById('close-remove-milestone-modal-btn');
    const cancelRemoveMilestoneBtn = document.getElementById('cancel-remove-milestone-btn');
    const confirmRemoveMilestoneBtn = document.getElementById('confirm-remove-milestone-btn');
    let milestoneIdToRemove = null;

    const closeRemoveMilestoneModal = () => {
        UI.toggleElement('remove-milestone-modal', false);
        document.body.style.overflow = '';
        UI.hideMessage('error-message');
        milestoneIdToRemove = null;
    };

    if (closeRemoveMilestoneModalBtn) closeRemoveMilestoneModalBtn.addEventListener('click', closeRemoveMilestoneModal);
    if (cancelRemoveMilestoneBtn) cancelRemoveMilestoneBtn.addEventListener('click', closeRemoveMilestoneModal);
    if (removeMilestoneModal) {
        removeMilestoneModal.addEventListener('click', (e) => {
            if (e.target === removeMilestoneModal) closeRemoveMilestoneModal();
        });
    }

    if (confirmRemoveMilestoneBtn) {
        confirmRemoveMilestoneBtn.addEventListener('click', async () => {
            if (!milestoneIdToRemove) return;
            
            UI.hideMessage('error-message');
            UI.setButtonLoading('confirm-remove-milestone-btn', true);

            try {
                await client.removeMilestone(roadmapId, milestoneIdToRemove);
                closeRemoveMilestoneModal();
                UI.setButtonLoading('confirm-remove-milestone-btn', false);
                await loadMilestones();
                UI.showSuccess('Milestone removed successfully');
            } catch (err) {
                console.error('Remove milestone failed:', err);
                UI.showError(err.message || 'Failed to remove milestone');
                UI.setButtonLoading('confirm-remove-milestone-btn', false);
            }
        });
    }

    window.openRemoveMilestoneModal = (id, name) => {
        milestoneIdToRemove = id;
        document.getElementById('milestone-to-remove-name').textContent = name;
        UI.toggleElement('remove-milestone-modal', true);
        document.body.style.overflow = 'hidden';
    };

    loadMilestones();
});

async function searchSubjects(searchToken) {
    UI.toggleElement('search-results-loading', true);
    UI.toggleElement('search-results', false);
    UI.toggleElement('no-search-results', false);

    try {
        const results = await client.searchSubjects(searchToken);

        UI.toggleElement('search-results-loading', false);

        if (results.length === 0) {
            UI.toggleElement('no-search-results', true);
        } else {
            UI.toggleElement('search-results', true);
            renderSearchResults(results);
        }
    } catch (err) {
        console.error('Search subjects failed:', err);
        UI.toggleElement('search-results-loading', false);
        UI.showError('Failed to search subjects: ' + (err.message || 'Unknown error'));
    }
}

function renderSearchResults(subjects) {
    const container = document.getElementById('search-results');
    container.innerHTML = '';

    subjects.forEach((subject, index) => {
        const resultItem = document.createElement('div');
        resultItem.className = 'search-result-item';
        resultItem.style.cursor = 'pointer';
        resultItem.innerHTML = `
            <div class="search-result-label" style="pointer-events: none;">
                <input 
                    type="radio" 
                    name="subject-select" 
                    value="${subject.id}"
                    ${index === 0 ? 'checked' : ''}
                    style="pointer-events: auto;"
                >
                <div>
                    <div class="search-result-name">${UI.escapeHtml(subject.name)}</div>
                </div>
            </div>
        `;

        resultItem.addEventListener('click', (e) => {
            const radio = resultItem.querySelector('input[type="radio"]');
            if (radio && e.target !== radio) {
                radio.checked = true;
            }
            
            // Highlight the selected item
            container.querySelectorAll('.search-result-item').forEach(item => {
                item.classList.remove('selected');
            });
            resultItem.classList.add('selected');
        });

        // Set initial selected state
        if (index === 0) {
            resultItem.classList.add('selected');
        }

        container.appendChild(resultItem);
    });
}

function clearSearchResults() {
    document.getElementById('subject-search-input').value = '';
    document.getElementById('search-results').innerHTML = '';
    UI.toggleElement('search-results', false);
    UI.toggleElement('no-search-results', false);
    UI.toggleElement('search-results-loading', false);
}

async function loadMilestones() {
    UI.toggleElement('loading', true);
    UI.toggleElement('milestones-container', false);
    UI.toggleElement('empty-state', false);

    try {
        const roadmapId = UI.getUrlParam('id');
        const roadmapName = UI.getUrlParam('name');
        const response = await client.getMilestones(roadmapId);

        document.getElementById('roadmap-name').textContent = roadmapName;
        document.title = `${roadmapName || 'Roadmap'} - Flashback`;


        UI.toggleElement('loading', false);

        if (response.milestones.length === 0) {
            UI.toggleElement('empty-state', true);
            // Hide search container if there are no milestones at all
            const searchContainer = document.querySelector('.search-container');
            if (searchContainer) searchContainer.style.display = 'none';
        } else {
            UI.toggleElement('milestones-container', true);
            // Show search container if we have milestones (and actions are shown)
            const renameBtn = document.getElementById('rename-roadmap-btn');
            const searchContainer = document.querySelector('.search-container');
            if (searchContainer && renameBtn && renameBtn.style.display !== 'none') {
                searchContainer.style.display = 'block';
            }
            renderMilestones(response.milestones);
            
            // Re-apply search if exists
            const searchInput = document.getElementById('milestone-search-input');
            if (searchInput && searchInput.value) {
                filterMilestones(searchInput.value.toLowerCase().trim());
            }
        }
    } catch (err) {
        console.error('Loading milestones failed:', err);
        UI.toggleElement('loading', false);
        UI.showError('Failed to load milestones: ' + (err.message || 'Unknown error'));
    }
}

function renderMilestones(milestones) {
    currentMilestones = milestones;
    const container = document.getElementById('milestones-container');
    container.innerHTML = '';

    const levelNames = ['Surface', 'Depth', 'Origin'];

    milestones.forEach((milestone, index) => {
        const milestoneCard = document.createElement('div');
        milestoneCard.className = 'item-block compact';
        milestoneCard.dataset.position = milestone.position;


        const startLongPressTimer = (e) => {
            if (reorderState.active) return;
            const touch = e.touches ? e.touches[0] : e;
            reorderState.startPos = { x: touch.clientX, y: touch.clientY };
            reorderState.longPressTimer = setTimeout(() => {
                enterReorderMode(index);
            }, 500);
        };

        const clearLongPressTimer = () => {
            if (reorderState.longPressTimer) {
                clearTimeout(reorderState.longPressTimer);
                reorderState.longPressTimer = null;
            }
        };

        milestoneCard.addEventListener('mousedown', (e) => {
            if (e.target.closest('button') || e.target.closest('select') || e.target.closest('input')) return;
            startLongPressTimer(e);
        });
        milestoneCard.addEventListener('mouseup', clearLongPressTimer);
        milestoneCard.addEventListener('mouseleave', clearLongPressTimer);
        milestoneCard.addEventListener('touchstart', (e) => {
            if (e.target.closest('button') || e.target.closest('select') || e.target.closest('input')) return;
            startLongPressTimer(e);
        }, { passive: true });
        milestoneCard.addEventListener('touchend', clearLongPressTimer);
        milestoneCard.addEventListener('touchmove', (e) => {
            if (reorderState.longPressTimer) {
                const touch = e.touches[0];
                const dx = touch.clientX - reorderState.startPos.x;
                const dy = touch.clientY - reorderState.startPos.y;
                if (Math.sqrt(dx * dx + dy * dy) > 10) {
                    clearLongPressTimer();
                }
            }
        }, { passive: true });
        milestoneCard.addEventListener('touchcancel', clearLongPressTimer);

        milestoneCard.innerHTML = `
            <div class="item-header" style="margin-bottom: 0; align-items: center; pointer-events: none; overflow: hidden; flex-wrap: wrap; gap: 0.75rem;">
                <div style="display: flex; align-items: center; gap: var(--space-xs); flex: 1; min-width: 250px;" class="milestone-link">
                    <span class="item-badge" style="font-size: 10px; height: 18px; min-width: 18px; padding: 0 4px; text-align: center; flex-shrink: 0; margin-top: 2px;">${index + 1}</span>
                    <h3 class="item-title" style="margin: 0; font-size: var(--font-size-base); line-height: 1.4;">${UI.escapeHtml(milestone.name)}</h3>
                </div>
                <div style="display: flex; align-items: center; gap: 0.5rem; pointer-events: auto; flex-shrink: 0; margin-left: auto;">
                    <select class="milestone-level-selector" data-id="${milestone.id}" data-current-level="${milestone.level}" title="Change level">
                        <option value="0" ${milestone.level === 0 ? 'selected' : ''}>Surface</option>
                        <option value="1" ${milestone.level === 1 ? 'selected' : ''}>Depth</option>
                        <option value="2" ${milestone.level === 2 ? 'selected' : ''}>Origin</option>
                    </select>
                    <button class="milestone-remove-btn" data-id="${milestone.id}" title="Remove milestone">×</button>
                </div>
            </div>
        `;

        // Selection-based reorder click handler
        milestoneCard.style.cursor = 'pointer';
        milestoneCard.addEventListener('click', async (e) => {
            if (reorderState.preventClick) {
                reorderState.preventClick = false;
                return;
            }
            if (reorderState.active) {
                if (reorderState.sourceIndex === index) {
                    exitReorderMode();
                    return;
                }

                const sourceMilestone = currentMilestones[reorderState.sourceIndex];
                if (!sourceMilestone) {
                    console.error('Source milestone not found at index:', reorderState.sourceIndex);
                    exitReorderMode();
                    return;
                }

                const sourcePos = parseInt(sourceMilestone.position);
                const targetPos = parseInt(milestone.position);

                window.showConfirmModal('Confirm Reorder', 'Are you sure you want to move this subject here?', async () => {
                    await reorderMilestone(sourcePos, targetPos);
                    exitReorderMode();
                });
                return;
            }

            if (!e.target.closest('button') && !e.target.closest('select') && !e.target.closest('input')) {
                const roadmapId = UI.getUrlParam('id');
                const roadmapName = UI.getUrlParam('name');
                window.location.href = `subject.html?id=${milestone.id}&name=${encodeURIComponent(milestone.name)}&level=${milestone.level}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName || '')}`;
            }
        });

        // Remove old drag event listeners
        milestoneCard.addEventListener('touchcancel', clearLongPressTimer);

        // Level selector handler
        const levelSelector = milestoneCard.querySelector('.milestone-level-selector');
        if (levelSelector) {
            levelSelector.addEventListener('click', (e) => {
                e.stopPropagation(); // Prevent card click navigation
            });

            levelSelector.addEventListener('change', async (e) => {
                e.stopPropagation(); // Prevent card click navigation
                const newLevel = parseInt(e.target.value);
                const currentLevel = parseInt(e.target.dataset.currentLevel);

                if (newLevel !== currentLevel) {
                    await changeMilestoneLevel(milestone.id, newLevel);
                }
            });
        }

        // Remove milestone handler
        const removeBtn = milestoneCard.querySelector('.milestone-remove-btn');
        if (removeBtn) {
            removeBtn.addEventListener('click', (e) => {
                e.stopPropagation(); // Prevent card click navigation
                window.openRemoveMilestoneModal(milestone.id, milestone.name);
            });
        }

        container.appendChild(milestoneCard);
    });
}

async function reorderMilestone(currentPosition, targetPosition) {
    const roadmapId = UI.getUrlParam('id');

    try {
        await client.reorderMilestone(roadmapId, currentPosition, targetPosition);
        await loadMilestones();
    } catch (err) {
        console.error('Reorder milestone failed:', err);
        UI.showError('Failed to reorder milestone: ' + (err.message || 'Unknown error'));
    }
}

async function removeMilestone(milestoneId) {
    const roadmapId = UI.getUrlParam('id');

    try {
        await client.removeMilestone(roadmapId, milestoneId);
        await loadMilestones();
        UI.showSuccess('Milestone removed successfully');
    } catch (err) {
        console.error('Remove milestone failed:', err);
        UI.showError('Failed to remove milestone: ' + (err.message || 'Unknown error'));
    }
}

async function changeMilestoneLevel(milestoneId, newLevel) {
    const roadmapId = UI.getUrlParam('id');

    try {
        await client.changeMilestoneLevel(roadmapId, milestoneId, newLevel);
        await loadMilestones();
        UI.showSuccess('Milestone level updated successfully');
    } catch (err) {
        console.error('Change milestone level failed:', err);
        UI.showError('Failed to change milestone level: ' + (err.message || 'Unknown error'));
    }
}

function filterMilestones(searchTerm) {
    const container = document.getElementById('milestones-container');
    const milestoneCards = container.querySelectorAll('.item-block');
    let hasResults = false;

    milestoneCards.forEach(card => {
        const titleElement = card.querySelector('.item-title');
        if (!titleElement) return;
        const name = titleElement.textContent.toLowerCase();
        if (name.includes(searchTerm)) {
            card.style.display = 'block';
            hasResults = true;
        } else {
            card.style.display = 'none';
        }
    });

    UI.toggleElement('no-milestones-found', !hasResults && searchTerm !== '');
    
    // Hide empty state if we're searching, even if no results found in search
    if (searchTerm !== '') {
        UI.toggleElement('empty-state', false);
    } else if (currentMilestones.length === 0) {
        UI.toggleElement('empty-state', true);
    }
}
