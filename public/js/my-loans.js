// LibroSphere Member Portal (My Loans)

const MyLoans = {
    async init() {
        if (!Auth.isLoggedIn()) {
            showToast('Please sign in to view your borrowed books.', 'info');
            setTimeout(() => { window.location.href = '/login.html'; }, 800);
            return;
        }

        await this.loadMyLoans();
    },

    async loadMyLoans() {
        const activeContainer = document.getElementById('active-loans-list');
        const historyContainer = document.getElementById('loans-history-tbody');
        if (!activeContainer) return;

        activeContainer.innerHTML = '<p style="color: var(--text-muted); padding: 1.5rem;">Loading your active loans...</p>';

        try {
            const res = await API.get('/api/loans/my-loans');
            if (res.success && res.data) {
                const { user, activeLoans, history } = res.data;
                this.renderMemberHeader(user, activeLoans);
                this.renderActiveLoans(activeLoans);
                this.renderHistory(history);
            }
        } catch (err) {
            activeContainer.innerHTML = `
                <div style="padding: 2rem; color: var(--danger);">
                    <p>Failed to load your circulation records: ${err.message}</p>
                </div>
            `;
        }
    },

    renderMemberHeader(user, activeLoans) {
        const nameEl = document.getElementById('member-name');
        const codeEl = document.getElementById('member-code');
        const emailEl = document.getElementById('member-email');
        const countEl = document.getElementById('member-active-count');
        const finesEl = document.getElementById('member-fines-due');

        if (nameEl) nameEl.innerText = user.name;
        if (codeEl) codeEl.innerText = user.memberCode;
        if (emailEl) emailEl.innerText = user.email;
        if (countEl) countEl.innerText = activeLoans.length;

        let totalFines = 0.0;
        activeLoans.forEach(l => {
            if (l.fineAmount > 0 && !l.finePaid) totalFines += l.fineAmount;
        });

        if (finesEl) {
            finesEl.innerText = `$${totalFines.toFixed(2)}`;
            finesEl.style.color = totalFines > 0 ? 'var(--danger)' : 'var(--success)';
        }
    },

    renderActiveLoans(loans) {
        const container = document.getElementById('active-loans-list');
        if (!container) return;

        if (loans.length === 0) {
            container.innerHTML = `
                <div style="text-align: center; padding: 3.5rem 1rem; background: white; border-radius: var(--radius-lg); border: 1px dashed var(--border-light);">
                    <div style="font-size: 3rem; margin-bottom: 0.5rem;">📚</div>
                    <h3 style="font-size: 1.25rem; font-weight: 700; margin-bottom: 0.5rem;">No Books Currently Borrowed</h3>
                    <p style="color: var(--text-muted); margin-bottom: 1.5rem;">Explore our curated catalog and borrow your next read today.</p>
                    <a href="/index.html#catalog-section" class="btn btn-primary">Browse Library Catalog</a>
                </div>
            `;
            return;
        }

        container.innerHTML = loans.map(l => {
            const isOverdue = l.isOverdue || l.daysLeft < 0;
            const daysAbs = Math.abs(l.daysLeft);
            const statusBadge = isOverdue
                ? `<span class="badge badge-overdue">⚠️ OVERDUE by ${daysAbs} day${daysAbs === 1 ? '' : 's'}</span>`
                : `<span class="badge badge-borrowed">⏳ ${l.daysLeft} day${l.daysLeft === 1 ? '' : 's'} remaining</span>`;

            const fineHtml = (l.fineAmount > 0 && !l.finePaid)
                ? `<div style="font-size: 0.85rem; color: var(--danger); font-weight: 700; margin-top: 0.25rem;">Accrued Fine: $${l.fineAmount.toFixed(2)}</div>`
                : '';

            const fallbackCover = 'https://images.unsplash.com/photo-1544716278-ca5e3f4abd8c?w=500&auto=format&fit=crop&q=80';
            const cover = l.bookCoverUrl || fallbackCover;

            return `
                <div class="desk-card" style="margin-bottom: 1rem; display: flex; flex-direction: row; align-items: center; justify-content: space-between; padding: 1.25rem; gap: 1.5rem; flex-wrap: wrap;">
                    <div style="display: flex; align-items: center; gap: 1.25rem;">
                        <img src="${cover}" alt="${l.bookTitle}" style="width: 60px; height: 80px; object-fit: cover; border-radius: var(--radius-sm); box-shadow: var(--shadow-sm);" onerror="this.src='${fallbackCover}'">
                        <div>
                            <h4 style="font-size: 1.1rem; font-weight: 700; margin-bottom: 0.2rem;">${l.bookTitle}</h4>
                            <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 0.4rem;">by ${l.bookAuthor} &bull; ISBN: ${l.bookIsbn}</p>
                            <div style="display: flex; align-items: center; gap: 0.75rem; flex-wrap: wrap;">
                                ${statusBadge}
                                <span style="font-size: 0.8rem; color: var(--text-subtle);">Borrowed: ${l.borrowDate.substring(0, 10)}</span>
                                <span style="font-size: 0.8rem; color: var(--text-subtle);">Due: ${l.dueDate.substring(0, 10)}</span>
                            </div>
                            ${fineHtml}
                        </div>
                    </div>
                    <div style="display: flex; gap: 0.75rem; align-items: center;">
                        <button class="btn btn-primary btn-sm" onclick="MyLoans.returnBook(${l.id})">
                            Return Book
                        </button>
                    </div>
                </div>
            `;
        }).join('');
    },

    renderHistory(history) {
        const tbody = document.getElementById('loans-history-tbody');
        if (!tbody) return;

        if (history.length === 0) {
            tbody.innerHTML = `<tr><td colspan="6" style="text-align: center; color: var(--text-muted); padding: 2rem;">No past loans recorded.</td></tr>`;
            return;
        }

        tbody.innerHTML = history.map(h => {
            const statusBadge = h.status === 'returned'
                ? `<span class="badge badge-returned">Returned</span>`
                : (h.status === 'overdue' ? `<span class="badge badge-overdue">Overdue</span>` : `<span class="badge badge-borrowed">Active</span>`);

            const fineBadge = h.fineAmount > 0
                ? (h.finePaid ? `<span style="color: var(--success); font-weight: 600;">$${h.fineAmount.toFixed(2)} (Paid)</span>` : `<span style="color: var(--danger); font-weight: 700;">$${h.fineAmount.toFixed(2)} (Unpaid)</span>`)
                : `<span style="color: var(--text-subtle);">$0.00</span>`;

            return `
                <tr>
                    <td><strong>${h.bookTitle}</strong><div style="font-size: 0.75rem; color: var(--text-subtle);">${h.bookAuthor}</div></td>
                    <td>${h.borrowDate ? h.borrowDate.substring(0, 10) : '-'}</td>
                    <td>${h.dueDate ? h.dueDate.substring(0, 10) : '-'}</td>
                    <td>${h.returnDate ? h.returnDate.substring(0, 10) : '<em style="color: var(--primary);">Active</em>'}</td>
                    <td>${statusBadge}</td>
                    <td>${fineBadge}</td>
                </tr>
            `;
        }).join('');
    },

    async returnBook(loanId) {
        if (!confirm('Are you sure you want to return this book?')) return;

        try {
            const res = await API.post(`/api/loans/return/${loanId}`, {});
            if (res.success) {
                showToast('Book returned successfully to the library catalog!', 'success');
                await this.loadMyLoans();
            }
        } catch (err) {
            showToast(err.message, 'error');
        }
    }
};

document.addEventListener('DOMContentLoaded', () => {
    MyLoans.init();
});
