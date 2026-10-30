// LibroSphere Authentication & Session Management

const Auth = {
    async init() {
        const token = API.getToken();
        if (token) {
            try {
                const res = await API.get('/api/auth/me');
                if (res.success && res.data && res.data.user) {
                    API.setUser(res.data.user);
                }
            } catch (err) {
                console.warn('Session verification failed:', err.message);
                API.setToken(null);
                API.setUser(null);
            }
        }
        this.renderNav();
    },

    getCurrentUser() {
        return API.getUser();
    },

    isLoggedIn() {
        return !!API.getUser();
    },

    isStaff() {
        const u = API.getUser();
        return u && (u.role === 'admin' || u.role === 'librarian');
    },

    isAdmin() {
        const u = API.getUser();
        return u && u.role === 'admin';
    },

    async login(identifier, password) {
        const res = await API.post('/api/auth/login', { identifier, password });
        if (res.success && res.data) {
            API.setToken(res.data.token);
            API.setUser(res.data.user);
            showToast(`Welcome back, ${res.data.user.name}!`, 'success');
            this.renderNav();
            return res.data.user;
        }
        throw new Error(res.error || 'Login failed');
    },

    async register(name, email, password, phone, role = 'member') {
        const res = await API.post('/api/auth/register', { name, email, password, phone, role });
        if (res.success && res.data) {
            API.setToken(res.data.token);
            API.setUser(res.data.user);
            showToast(`Registration successful! Welcome, ${res.data.user.name}`, 'success');
            this.renderNav();
            return res.data.user;
        }
        throw new Error(res.error || 'Registration failed');
    },

    async logout() {
        try {
            await API.post('/api/auth/logout', {});
        } catch (e) {}
        API.setToken(null);
        API.setUser(null);
        showToast('Signed out successfully.', 'info');
        this.renderNav();
        setTimeout(() => {
            window.location.href = '/index.html';
        }, 300);
    },

    async demoLogin(role) {
        let creds = { identifier: 'alice_reader', password: 'alice123' };
        if (role === 'admin') {
            creds = { identifier: 'admin', password: 'admin123' };
        } else if (role === 'librarian') {
            creds = { identifier: 'sarah_lib', password: 'lib123' };
        }

        try {
            const user = await this.login(creds.identifier, creds.password);
            if (role === 'admin' || role === 'librarian') {
                window.location.href = '/admin.html';
            } else {
                window.location.href = '/my-loans.html';
            }
        } catch (err) {
            showToast(err.message, 'error');
        }
    },

    renderNav() {
        const authContainer = document.getElementById('nav-auth-container');
        if (!authContainer) return;

        const user = this.getCurrentUser();
        const staffNav = document.getElementById('nav-admin-link');
        const myLoansNav = document.getElementById('nav-myloans-link');

        if (staffNav) {
            staffNav.style.display = this.isStaff() ? 'flex' : 'none';
        }
        if (myLoansNav) {
            myLoansNav.style.display = user ? 'flex' : 'none';
        }

        if (user) {
            const initials = user.name ? user.name.split(' ').map(n => n[0]).join('').substring(0, 2).toUpperCase() : 'U';
            authContainer.innerHTML = `
                <div class="user-pill">
                    <div class="user-avatar">${initials}</div>
                    <div class="user-info">
                        <span class="user-name">${user.name}</span>
                        <span class="user-role-badge">${user.role} &bull; ${user.memberCode}</span>
                    </div>
                </div>
                <button class="btn btn-secondary btn-sm" onclick="Auth.logout()" title="Sign Out">
                    Sign Out
                </button>
            `;
        } else {
            authContainer.innerHTML = `
                <a href="/login.html" class="btn btn-outline btn-sm">Sign In</a>
                <a href="/register.html" class="btn btn-primary btn-sm">Join Library</a>
            `;
        }
    }
};

document.addEventListener('DOMContentLoaded', () => {
    Auth.init();
});
