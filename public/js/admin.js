// LibroSphere Staff & Admin Operations Hub

const Admin = {
    async init() {
        if (!Auth.isLoggedIn()) {
            showToast('Authentication required for staff access.', 'error');
            setTimeout(() => { window.location.href = '/login.html'; }, 800);
            return;
        }

        if (!Auth.isStaff()) {
            showToast('Access denied: Staff privileges required.', 'error');
            setTimeout(() => { window.location.href = '/index.html'; }, 1200);
            return;
        }

        this.bindTabs();
        await this.loadOverview();
        await this.loadCirculationDesk();
        await this.loadInventory();
        await this.loadMembers();
        await this.loadLoans();
    },

    bindTabs() {
        document.querySelectorAll('.admin-nav-item').forEach(btn => {
            btn.addEventListener('click', () => {
                const targetTab = btn.getAttribute('data-tab');
                document.querySelectorAll('.admin-nav-item').forEach(b => b.classList.remove('active'));
                document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));

                btn.classList.add('active');
                const pane = document.getElementById(`tab-${targetTab}`);
                if (pane) pane.classList.add('active');

                // Refresh corresponding data on tab change
                if (targetTab === 'overview') this.loadOverview();
                else if (targetTab === 'circulation') this.loadCirculationDesk();
                else if (targetTab === 'inventory') this.loadInventory();
                else if (targetTab === 'members') this.loadMembers();
                else if (targetTab === 'loans') this.loadLoans();
            });
        });
    },

    // 1. Overview Dashboard
    async loadOverview() {
        try {
            const res = await API.get('/api/dashboard/stats');
            if (res.success && res.data) {
                const s = res.data;
                const setVal = (id, val) => {
                    const el = document.getElementById(id);
                    if (el) el.innerText = val.toLocaleString();
                };
                setVal('ov-titles', s.totalTitles || 0);
                setVal('ov-copies', s.totalCopies || 0);
                setVal('ov-borrowed', s.borrowedCopies || 0);
                setVal('ov-active-loans', s.activeLoans || 0);
                setVal('ov-overdue-loans', s.overdueLoans || 0);
                setVal('ov-members', s.totalMembers || 0);

                const finesEl = document.getElementById('ov-fines');
                if (finesEl) finesEl.innerText = `$${(s.totalPendingFines || 0).toFixed(2)}`;

                // Recent Activities
                const feed = document.getElementById('admin-activity-feed');
                if (feed && s.recentActivities) {
                    if (s.recentActivities.length === 0) {
                        feed.innerHTML = '<li style="color: var(--text-muted); padding: 1rem;">No recent activities recorded.</li>';
                    } else {
                        feed.innerHTML = s.recentActivities.map(a => `
                            <li class="activity-item">
                                <div class="activity-badge">📌</div>
                                <div class="activity-content">
                                    <div class="activity-text"><strong>${a.userName}</strong>: ${a.details}</div>
                                    <div class="activity-time">${a.createdAt} &bull; ${a.action}</div>
                                </div>
                            </li>
                        `).join('');
                    }
                }
            }
        } catch (err) {
            console.error('Error loading dashboard stats:', err);
        }
    },

    // 2. Circulation Desk (Issue & Return)
    async loadCirculationDesk() {
        const bookSelect = document.getElementById('desk-issue-book');
        const userSelect = document.getElementById('desk-issue-user');

        try {
            // Load available books
            const booksRes = await API.get('/api/books?available=true');
            if (booksRes.success && booksRes.data && bookSelect) {
                bookSelect.innerHTML = '<option value="">-- Select Available Volume --</option>' +
                    booksRes.data.books.map(b => `
                        <option value="${b.id}">${b.title} (${b.availableCopies} avail) - ${b.isbn}</option>
                    `).join('');
            }

            // Load members
            const membersRes = await API.get('/api/members');
            if (membersRes.success && membersRes.data && userSelect) {
                userSelect.innerHTML = '<option value="">-- Select Member / Student --</option>' +
                    membersRes.data.members.filter(m => m.status === 'active').map(m => `
                        <option value="${m.id}">${m.name} (${m.memberCode}) - ${m.email}</option>
                    `).join('');
            }

            // Load active circulation desk returns
            const activeLoansRes = await API.get('/api/loans?status=borrowed&limit=20');
            const returnTbody = document.getElementById('desk-active-loans-tbody');
            if (returnTbody && activeLoansRes.success && activeLoansRes.data) {
                const loans = activeLoansRes.data.loans;
                if (loans.length === 0) {
                    returnTbody.innerHTML = '<tr><td colspan="5" style="text-align: center; color: var(--text-muted); padding: 1.5rem;">No active loans to return.</td></tr>';
                } else {
                    returnTbody.innerHTML = loans.map(l => {
                        const isOver = l.isOverdue || l.daysLeft < 0;
                        const badge = isOver
                            ? `<span class="badge badge-overdue">Overdue (${Math.abs(l.daysLeft)}d)</span>`
                            : `<span class="badge badge-borrowed">${l.daysLeft}d left</span>`;
                        return `
                            <tr>
                                <td><strong>${l.bookTitle}</strong></td>
                                <td>${l.userName} (${l.memberCode})</td>
                                <td>${l.dueDate.substring(0, 10)}</td>
                                <td>${badge}</td>
                                <td>
                                    <button class="btn btn-primary btn-sm" onclick="Admin.returnLoan(${l.id})">
                                        Return
                                    </button>
                                </td>
                            </tr>
                        `;
                    }).join('');
                }
            }
        } catch (err) {
            console.error('Error loading circulation desk:', err);
        }
    },

    async issueLoanFromDesk(e) {
        if (e) e.preventDefault();
        const bookId = parseInt(document.getElementById('desk-issue-book')?.value, 10);
        const userId = parseInt(document.getElementById('desk-issue-user')?.value, 10);
        const loanDays = parseInt(document.getElementById('desk-issue-days')?.value, 10) || 14;
        const notes = document.getElementById('desk-issue-notes')?.value || 'Issued by staff';

        if (!bookId || !userId) {
            showToast('Please select both a book and a member.', 'error');
            return;
        }

        try {
            const res = await API.post('/api/loans/issue', { bookId, userId, loanDays, notes });
            if (res.success) {
                showToast('Book successfully issued!', 'success');
                document.getElementById('desk-issue-form')?.reset();
                await this.loadCirculationDesk();
                await this.loadOverview();
            }
        } catch (err) {
            showToast(err.message, 'error');
        }
    },

    async returnLoan(loanId) {
        try {
            const res = await API.post(`/api/loans/return/${loanId}`, {});
            if (res.success) {
                showToast('Book successfully returned to stock!', 'success');
                await this.loadCirculationDesk();
                await this.loadOverview();
            }
        } catch (err) {
            showToast(err.message, 'error');
        }
    },

    // 3. Book Inventory Management
    async loadInventory() {
        const tbody = document.getElementById('admin-inventory-tbody');
        const searchVal = document.getElementById('admin-inventory-search')?.value.trim() || '';
        if (!tbody) return;

        tbody.innerHTML = '<tr><td colspan="7" style="text-align: center; color: var(--text-muted); padding: 2rem;">Loading inventory records...</td></tr>';

        try {
            const res = await API.get(`/api/books?q=${encodeURIComponent(searchVal)}`);
            if (res.success && res.data && res.data.books) {
                const books = res.data.books;
                if (books.length === 0) {
                    tbody.innerHTML = '<tr><td colspan="7" style="text-align: center; color: var(--text-muted); padding: 2rem;">No matching volumes found.</td></tr>';
                    return;
                }

                tbody.innerHTML = books.map(b => {
                    const availBadge = b.availableCopies > 0
                        ? `<span class="badge badge-active">${b.availableCopies} / ${b.totalCopies}</span>`
                        : `<span class="badge badge-suspended">0 / ${b.totalCopies}</span>`;

                    return `
                        <tr>
                            <td>
                                <strong>${b.title}</strong>
                                <div style="font-size: 0.75rem; color: var(--text-subtle);">ISBN: ${b.isbn}</div>
                            </td>
                            <td>${b.author}</td>
                            <td>${b.categoryName || '-'}</td>
                            <td>${b.shelfLocation || '-'}</td>
                            <td>${availBadge}</td>
                            <td>${b.publishYear}</td>
                            <td>
                                <div style="display: flex; gap: 0.4rem;">
                                    <button class="btn btn-secondary btn-sm" onclick="Admin.openEditBookModal(${b.id})">Edit</button>
                                    <button class="btn btn-danger btn-sm" onclick="Admin.deleteBook(${b.id}, '${b.title.replace(/'/g, "\\'")}')">Del</button>
                                </div>
                            </td>
                        </tr>
                    `;
                }).join('');
            }
        } catch (err) {
            tbody.innerHTML = `<tr><td colspan="7" style="text-align: center; color: var(--danger); padding: 2rem;">${err.message}</td></tr>`;
        }
    },

    openAddBookModal() {
        const form = document.getElementById('book-form');
        if (form) form.reset();
        document.getElementById('book-form-id').value = '';
        document.getElementById('book-modal-title').innerText = 'Add New Book to Collection';
        document.getElementById('admin-book-modal').classList.add('show');
    },

    async openEditBookModal(id) {
        try {
            const res = await API.get(`/api/books/${id}`);
            if (res.success && res.data && res.data.book) {
                const b = res.data.book;
                document.getElementById('book-form-id').value = b.id;
                document.getElementById('book-form-title').value = b.title;
                document.getElementById('book-form-author').value = b.author;
                document.getElementById('book-form-isbn').value = b.isbn;
                document.getElementById('book-form-category').value = b.categoryId || 1;
                document.getElementById('book-form-publisher').value = b.publisher || '';
                document.getElementById('book-form-year').value = b.publishYear || 2024;
                document.getElementById('book-form-copies').value = b.totalCopies || 1;
                document.getElementById('book-form-shelf').value = b.shelfLocation || '';
                document.getElementById('book-form-cover').value = b.coverUrl || '';
                document.getElementById('book-form-desc').value = b.description || '';

                document.getElementById('book-modal-title').innerText = 'Edit Book Details';
                document.getElementById('admin-book-modal').classList.add('show');
            }
        } catch (err) {
            showToast(err.message, 'error');
        }
    },

    closeBookModal() {
        document.getElementById('admin-book-modal')?.classList.remove('show');
    },

    async saveBook(e) {
        if (e) e.preventDefault();
        const id = document.getElementById('book-form-id').value;
        const payload = {
            title: document.getElementById('book-form-title').value.trim(),
            author: document.getElementById('book-form-author').value.trim(),
            isbn: document.getElementById('book-form-isbn').value.trim(),
            categoryId: parseInt(document.getElementById('book-form-category').value, 10) || 1,
            publisher: document.getElementById('book-form-publisher').value.trim(),
            publishYear: parseInt(document.getElementById('book-form-year').value, 10) || 2024,
            totalCopies: parseInt(document.getElementById('book-form-copies').value, 10) || 1,
            shelfLocation: document.getElementById('book-form-shelf').value.trim(),
            coverUrl: document.getElementById('book-form-cover').value.trim(),
            description: document.getElementById('book-form-desc').value.trim()
        };

        try {
            if (id) {
                await API.put(`/api/books/${id}`, payload);
                showToast('Book details updated successfully!', 'success');
            } else {
                await API.post('/api/books', payload);
                showToast('New book successfully cataloged!', 'success');
            }
            this.closeBookModal();
            await this.loadInventory();
            await this.loadOverview();
        } catch (err) {
            showToast(err.message, 'error');
        }
    },

    async deleteBook(id, title) {
        if (!confirm(`Are you sure you want to permanently delete "${title}"?`)) return;

        try {
            await API.delete(`/api/books/${id}`);
            showToast('Book removed from library.', 'success');
            await this.loadInventory();
            await this.loadOverview();
        } catch (err) {
            showToast(err.message, 'error');
        }
    },

    // 4. Member Management
    async loadMembers() {
        const tbody = document.getElementById('admin-members-tbody');
        const searchVal = document.getElementById('admin-members-search')?.value.trim() || '';
        if (!tbody) return;

        tbody.innerHTML = '<tr><td colspan="7" style="text-align: center; color: var(--text-muted); padding: 2rem;">Loading member directory...</td></tr>';

        try {
            const res = await API.get(`/api/members?q=${encodeURIComponent(searchVal)}`);
            if (res.success && res.data && res.data.members) {
                const members = res.data.members;
                if (members.length === 0) {
                    tbody.innerHTML = '<tr><td colspan="7" style="text-align: center; color: var(--text-muted); padding: 2rem;">No members found.</td></tr>';
                    return;
                }

                tbody.innerHTML = members.map(m => {
                    const statusBadge = m.status === 'active'
                        ? `<span class="badge badge-active">Active</span>`
                        : `<span class="badge badge-suspended">Suspended</span>`;

                    const roleBadge = m.role === 'admin'
                        ? `<span style="color: var(--primary); font-weight: 700;">Admin</span>`
                        : (m.role === 'librarian' ? `<span style="color: var(--accent); font-weight: 700;">Librarian</span>` : 'Member');

                    return `
                        <tr>
                            <td><code>${m.memberCode}</code></td>
                            <td><strong>${m.name}</strong></td>
                            <td>${m.email}</td>
                            <td>${roleBadge}</td>
                            <td>${m.activeLoansCount} active</td>
                            <td>${statusBadge}</td>
                            <td>
                                <div style="display: flex; gap: 0.4rem;">
                                    ${m.role !== 'admin' ? `
                                        <button class="btn btn-secondary btn-sm" onclick="Admin.toggleMemberStatus(${m.id}, '${m.status}')">
                                            ${m.status === 'active' ? 'Suspend' : 'Activate'}
                                        </button>
                                    ` : ''}
                                    ${Auth.isAdmin() && m.role !== 'admin' ? `
                                        <button class="btn btn-danger btn-sm" onclick="Admin.deleteMember(${m.id}, '${m.name.replace(/'/g, "\\'")}')">
                                            Del
                                        </button>
                                    ` : ''}
                                </div>
                            </td>
                        </tr>
                    `;
                }).join('');
            }
        } catch (err) {
            tbody.innerHTML = `<tr><td colspan="7" style="text-align: center; color: var(--danger); padding: 2rem;">${err.message}</td></tr>`;
        }
    },

    openAddMemberModal() {
        document.getElementById('member-form')?.reset();
        document.getElementById('admin-member-modal')?.classList.add('show');
    },

    closeMemberModal() {
        document.getElementById('admin-member-modal')?.classList.remove('show');
    },

    async saveMember(e) {
        if (e) e.preventDefault();
        const payload = {
            name: document.getElementById('member-form-name').value.trim(),
            email: document.getElementById('member-form-email').value.trim(),
            password: document.getElementById('member-form-password').value.trim() || 'welcome123',
            phone: document.getElementById('member-form-phone').value.trim(),
            role: document.getElementById('member-form-role').value
        };

        try {
            await API.post('/api/members', payload);
            showToast('New member registered successfully!', 'success');
            this.closeMemberModal();
            await this.loadMembers();
            await this.loadOverview();
        } catch (err) {
            showToast(err.message, 'error');
        }
    },

    async toggleMemberStatus(id, currentStatus) {
        const newStatus = currentStatus === 'active' ? 'suspended' : 'active';
        try {
            await API.put(`/api/members/${id}/status`, { status: newStatus });
            showToast(`Member marked as ${newStatus}.`, 'success');
            await this.loadMembers();
        } catch (err) {
            showToast(err.message, 'error');
        }
    },

    async deleteMember(id, name) {
        if (!confirm(`Are you sure you want to delete member ${name}?`)) return;
        try {
            await API.delete(`/api/members/${id}`);
            showToast('Member removed.', 'success');
            await this.loadMembers();
            await this.loadOverview();
        } catch (err) {
            showToast(err.message, 'error');
        }
    },

    // 5. Circulation Records Tracker
    async loadLoans() {
        const tbody = document.getElementById('admin-loans-tbody');
        const filterStatus = document.getElementById('admin-loans-filter')?.value || '';
        if (!tbody) return;

        tbody.innerHTML = '<tr><td colspan="7" style="text-align: center; color: var(--text-muted); padding: 2rem;">Loading circulation history...</td></tr>';

        try {
            const res = await API.get(`/api/loans?status=${filterStatus}&limit=100`);
            if (res.success && res.data && res.data.loans) {
                const loans = res.data.loans;
                if (loans.length === 0) {
                    tbody.innerHTML = '<tr><td colspan="7" style="text-align: center; color: var(--text-muted); padding: 2rem;">No loan records found for this filter.</td></tr>';
                    return;
                }

                tbody.innerHTML = loans.map(l => {
                    const isOver = l.isOverdue || l.daysLeft < 0;
                    let badge = `<span class="badge badge-borrowed">Active (${l.daysLeft}d)</span>`;
                    if (l.status === 'returned') badge = `<span class="badge badge-returned">Returned</span>`;
                    else if (isOver) badge = `<span class="badge badge-overdue">Overdue (${Math.abs(l.daysLeft)}d)</span>`;

                    const fineBadge = l.fineAmount > 0
                        ? (l.finePaid ? `<span style="color: var(--success); font-weight: 600;">$${l.fineAmount.toFixed(2)} (Paid)</span>` : `<span style="color: var(--danger); font-weight: 700;">$${l.fineAmount.toFixed(2)} (Pending)</span>`)
                        : `<span style="color: var(--text-subtle);">$0.00</span>`;

                    return `
                        <tr>
                            <td><strong>${l.bookTitle}</strong><div style="font-size: 0.75rem; color: var(--text-subtle);">${l.bookIsbn}</div></td>
                            <td>${l.userName}<div style="font-size: 0.75rem; color: var(--text-subtle);">${l.memberCode}</div></td>
                            <td>${l.borrowDate.substring(0, 10)}</td>
                            <td>${l.dueDate.substring(0, 10)}</td>
                            <td>${l.returnDate ? l.returnDate.substring(0, 10) : '-'}</td>
                            <td>${badge}</td>
                            <td>
                                <div style="display: flex; gap: 0.4rem; align-items: center;">
                                    ${fineBadge}
                                    ${l.status !== 'returned' ? `
                                        <button class="btn btn-primary btn-sm" onclick="Admin.returnLoan(${l.id})">Return</button>
                                    ` : ''}
                                </div>
                            </td>
                        </tr>
                    `;
                }).join('');
            }
        } catch (err) {
            tbody.innerHTML = `<tr><td colspan="7" style="text-align: center; color: var(--danger); padding: 2rem;">${err.message}</td></tr>`;
        }
    }
};

document.addEventListener('DOMContentLoaded', () => {
    Admin.init();
});
