<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MBPSAAS - Android Mobile Application Prototype &amp; UI Wireframe Specification</title>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&family=JetBrains+Mono:wght@400;500;600&display=swap" rel="stylesheet">
    <style>
        :root {
            --mbp-primary: #1b5e20;
            --mbp-primary-dark: #0f3d14;
            --mbp-emerald: #2e7d32;
            --mbp-emerald-light: #e8f5e9;
            --mbp-crimson: #c62828;
            --mbp-crimson-light: #ffebee;
            --mbp-amber: #e65100;
            --mbp-amber-light: #fff3e0;
            --mbp-blue: #0288d1;
            --mbp-blue-light: #e1f5fe;
            --mbp-ink: #102a16;
            --mbp-muted: #546e5a;
            --mbp-border: #81c784;
            --mbp-border-light: #c8e6c9;
            --mbp-bg-subtle: #f4fbf5;
        }

        /* Color Scheme Modes */
        body.color-theme {
            --frame-border: #1b5e20;
            --frame-inner-border: #2e7d32;
            --banner-bg: #1b5e20;
            --banner-text: #ffffff;
            --header-sub-bg: #e8f5e9;
            --header-sub-text: #0f3d14;
            --input-bg: #f4fbf5;
            --input-border: #81c784;
            --btn-primary-bg: #2e7d32;
            --btn-primary-text: #ffffff;
            --btn-secondary-bg: #ffffff;
            --btn-secondary-text: #1b5e20;
            --btn-secondary-border: #1b5e20;
            --arrow-color: #1b5e20;
            --badge-safe-bg: #d1fae5;
            --badge-safe-text: #065f46;
            --badge-safe-border: #10b981;
            --badge-alert-bg: #fee2e2;
            --badge-alert-text: #991b1b;
            --badge-alert-border: #ef4444;
            --stat-card-bg: #f1f8f3;
            --table-th-bg: #e8f5e9;
            --table-th-text: #0f3d14;
            --highlight-bg: #e0f2e3;
        }

        body.monochrome-theme {
            --frame-border: #000000;
            --frame-inner-border: #000000;
            --banner-bg: #ffffff;
            --banner-text: #000000;
            --header-sub-bg: #ffffff;
            --header-sub-text: #000000;
            --input-bg: #ffffff;
            --input-border: #000000;
            --btn-primary-bg: #ffffff;
            --btn-primary-text: #000000;
            --btn-secondary-bg: #ffffff;
            --btn-secondary-text: #000000;
            --btn-secondary-border: #000000;
            --arrow-color: #000000;
            --badge-safe-bg: #ffffff;
            --badge-safe-text: #000000;
            --badge-safe-border: #000000;
            --badge-alert-bg: #ffffff;
            --badge-alert-text: #000000;
            --badge-alert-border: #000000;
            --stat-card-bg: #ffffff;
            --table-th-bg: #ffffff;
            --table-th-text: #000000;
            --highlight-bg: #f1f5f9;
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }

        body {
            font-family: 'Inter', system-ui, -apple-system, sans-serif;
            background-color: #444c46;
            color: #102a16;
            -webkit-print-color-adjust: exact;
            print-color-adjust: exact;
        }

        /* Screen Control Bar (Sticky when previewed in browser) */
        .no-print-toolbar {
            position: sticky;
            top: 0;
            z-index: 9999;
            background: #0f2d14;
            color: #ffffff;
            padding: 12px 24px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            box-shadow: 0 4px 15px rgba(0,0,0,0.4);
            border-bottom: 2px solid var(--mbp-emerald);
        }

        .toolbar-brand {
            display: flex;
            align-items: center;
            gap: 12px;
            font-weight: 700;
            font-size: 1.02rem;
            letter-spacing: 0.5px;
        }

        .toolbar-badge {
            background: var(--mbp-emerald);
            color: #fff;
            padding: 3px 8px;
            border-radius: 4px;
            font-size: 0.72rem;
            text-transform: uppercase;
            letter-spacing: 1px;
            font-weight: 700;
        }

        .toolbar-actions {
            display: flex;
            align-items: center;
            gap: 12px;
        }

        .toolbar-btn {
            background: #1b5e20;
            color: #fff;
            border: 1px solid #81c784;
            padding: 7px 15px;
            border-radius: 6px;
            font-size: 0.82rem;
            font-weight: 600;
            cursor: pointer;
            display: inline-flex;
            align-items: center;
            gap: 7px;
            transition: all 0.2s ease;
            text-decoration: none;
        }

        .toolbar-btn:hover {
            background: #2e7d32;
            color: #fff;
        }

        .toolbar-btn-print {
            background: #2e7d32;
            border-color: #81c784;
        }
        .toolbar-btn-print:hover {
            background: #1b5e20;
        }

        .toolbar-select {
            background: #091a0c;
            color: #fff;
            border: 1px solid #2e7d32;
            padding: 7px 12px;
            border-radius: 6px;
            font-size: 0.82rem;
        }

        /* Printable Prototype Page Canvas */
        .prototype-canvas {
            display: flex;
            flex-direction: column;
            align-items: center;
            padding: 24px 0 60px;
            gap: 32px;
        }

        .prototype-page {
            width: 210mm;
            min-height: 297mm;
            padding: 14mm 16mm 14mm 16mm;
            background: #ffffff;
            box-shadow: 0 8px 24px rgba(0,0,0,0.3);
            display: flex;
            flex-direction: column;
            justify-content: flex-start;
            position: relative;
            page-break-after: always;
            break-after: page;
        }

        /* Top Header */
        .proto-header {
            text-align: center;
            margin-bottom: 12px;
        }

        .proto-header .proto-tag {
            font-size: 0.85rem;
            font-weight: 800;
            letter-spacing: 2px;
            color: var(--frame-border);
            text-transform: uppercase;
            margin-bottom: 2px;
        }

        .proto-header .proto-title {
            font-size: 0.78rem;
            font-weight: 700;
            letter-spacing: 1.5px;
            color: #102a16;
            text-transform: uppercase;
        }

        /* Main Double-Bordered Wireframe Containers */
        .wire-frame-box {
            border: 2px solid var(--frame-border);
            padding: 3px;
            background: #ffffff;
            margin-bottom: 2px;
        }

        .wire-frame-inner {
            border: 1px solid var(--frame-inner-border);
            padding: 12px 14px;
            display: flex;
            flex-direction: column;
            gap: 10px;
        }

        /* Banner title box */
        .wire-banner {
            border: 1.5px solid var(--frame-border);
            background: var(--banner-bg);
            color: var(--banner-text);
            text-align: center;
            padding: 6px 12px;
            font-size: 0.82rem;
            font-weight: 700;
            letter-spacing: 0.5px;
        }

        /* Subheader tabs bar */
        .wire-tabs {
            display: flex;
            border: 1px solid var(--frame-border);
            text-align: center;
            font-size: 0.74rem;
            font-weight: 600;
            overflow: hidden;
        }

        .wire-tab {
            flex: 1;
            padding: 5px 8px;
            background: var(--header-sub-bg);
            color: var(--header-sub-text);
            border-right: 1px solid var(--frame-border);
        }

        .wire-tab:last-child {
            border-right: none;
        }

        .wire-tab.active {
            background: var(--banner-bg);
            color: #ffffff;
        }

        /* Form Row & Field Layout */
        .wire-form-row {
            display: flex;
            align-items: center;
            gap: 12px;
            font-size: 0.74rem;
        }

        .wire-label {
            width: 155px;
            font-weight: 600;
            color: #102a16;
            flex-shrink: 0;
        }

        .wire-input {
            flex: 1;
            height: 28px;
            border: 1px solid var(--input-border);
            background: var(--input-bg);
            padding: 0 10px;
            font-size: 0.72rem;
            display: flex;
            align-items: center;
            justify-content: space-between;
            color: #27382b;
            border-radius: 2px;
        }

        .wire-input-val {
            font-family: 'Inter', sans-serif;
        }

        .wire-input-code {
            font-family: 'JetBrains Mono', monospace;
            font-weight: 600;
            letter-spacing: 0.5px;
        }

        .wire-input-icon {
            font-size: 0.75rem;
            color: var(--mbp-muted);
        }

        /* Flow Arrow Between Panels */
        .wire-arrow-down {
            text-align: center;
            font-size: 1.35rem;
            line-height: 1;
            color: var(--arrow-color);
            margin: 9px 0;
            font-weight: 800;
        }

        /* Button elements */
        .wire-btn {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            padding: 4px 14px;
            font-size: 0.72rem;
            font-weight: 600;
            border: 1px solid var(--frame-border);
            background: var(--btn-primary-bg);
            color: var(--btn-primary-text);
            border-radius: 2px;
            cursor: default;
        }

        .wire-btn-secondary {
            background: var(--btn-secondary-bg);
            color: var(--btn-secondary-text);
            border: 1px solid var(--btn-secondary-border);
        }

        .wire-btn-success {
            background: #2e7d32 !important;
            color: #ffffff !important;
            border-color: #1b5e20 !important;
        }

        .wire-btn-danger {
            background: #c62828 !important;
            color: #ffffff !important;
            border-color: #8e0000 !important;
        }

        .wire-btn-sm {
            padding: 2px 8px;
            font-size: 0.68rem;
        }

        .wire-action-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-top: 4px;
            font-size: 0.72rem;
        }

        .wire-link {
            color: #2e7d32;
            text-decoration: underline;
            font-size: 0.72rem;
            font-weight: 500;
        }

        /* Wireframe Table */
        .wire-table {
            width: 100%;
            border-collapse: collapse;
            border: 1px solid var(--frame-border);
            font-size: 0.69rem;
        }

        .wire-table th {
            border: 1px solid var(--frame-border);
            background: var(--table-th-bg);
            color: var(--table-th-text);
            padding: 5px 6px;
            font-weight: 700;
            text-align: center;
        }

        .wire-table td {
            border: 1px solid var(--frame-border);
            padding: 4px 6px;
            text-align: center;
            vertical-align: middle;
        }

        .wire-table td.text-left {
            text-align: left;
        }

        /* Badges */
        .wire-badge {
            display: inline-block;
            padding: 2px 7px;
            font-size: 0.64rem;
            font-weight: 700;
            border-radius: 3px;
            border: 1px solid;
            text-transform: uppercase;
        }

        .wire-badge-safe {
            background: var(--badge-safe-bg);
            color: var(--badge-safe-text);
            border-color: var(--badge-safe-border);
        }

        .wire-badge-alert {
            background: var(--badge-alert-bg);
            color: var(--badge-alert-text);
            border-color: var(--badge-alert-border);
        }

        .wire-badge-blue {
            background: #e1f5fe;
            color: #01579b;
            border-color: #0288d1;
        }

        /* Page Footer Indicator */
        .proto-footer {
            margin-top: auto;
            padding-top: 8px;
            border-top: 1px dashed var(--mbp-border-light);
            display: flex;
            justify-content: space-between;
            align-items: center;
            font-size: 0.65rem;
            color: var(--mbp-muted);
            font-weight: 500;
        }

        .proto-footer-brand {
            font-weight: 700;
            color: var(--frame-border);
        }

        /* Print Media Styles */
        @media print {
            body {
                background: #ffffff;
                margin: 0;
                padding: 0;
            }

            .no-print-toolbar {
                display: none !important;
            }

            .prototype-canvas {
                padding: 0;
                gap: 0;
            }

            .prototype-page {
                box-shadow: none;
                margin: 0;
                width: 100% !important;
                min-height: 100vh !important;
                padding: 12mm 14mm !important;
                page-break-after: always !important;
                break-after: page !important;
            }

            @page {
                size: A4 portrait;
                margin: 0;
            }
        }
    </style>
</head>
<body class="color-theme">

    <!-- Interactive Navigation Bar (Web View Only) -->
    <header class="no-print-toolbar">
        <div class="toolbar-brand">
            <span>MBPSAAS Android App Specification</span>
            <span class="toolbar-badge">Mobile Application Prototype</span>
        </div>
        <div class="toolbar-actions">
            <select class="toolbar-select" onchange="jumpToPage(this.value)">
                <option value="p1">Page 1: Mobile Login &amp; Account Lookup</option>
                <option value="p2">Page 2: Security Question &amp; Password Reset</option>
                <option value="p3">Page 3: Dashboard Screen (Normal Safe State)</option>
                <option value="p4">Page 4: Dashboard Screen (Intrusion Alert State)</option>
                <option value="p5">Page 5: PIR Sensor Control &amp; Zone Switches</option>
                <option value="p6">Page 6: Recent Activity Feed &amp; Motion Row</option>
                <option value="p7">Page 7: Triggered Alerts &amp; Date Filters</option>
                <option value="p8">Page 8: Filtered Alert Stream &amp; Empty States</option>
                <option value="p9">Page 9: Activity Log &amp; Operator Profile</option>
                <option value="p10">Page 10: Mobile Architecture &amp; Compose Design</option>
            </select>
            <button class="toolbar-btn" onclick="toggleTheme()">
                <span id="themeBtnText">🎨 Theme: Uniform App Colors</span>
            </button>
            <button class="toolbar-btn toolbar-btn-print" onclick="window.print()">
                <span>🖨️ Export / Print PDF (A4)</span>
            </button>
        </div>
    </header>

    <main class="prototype-canvas">

        <!-- ==========================================
             PAGE 1: MOBILE LOGIN & STEP 1 ACCOUNT LOOKUP
             ========================================== -->
        <section class="prototype-page" id="p1">
            <div class="proto-header">
                <div class="proto-tag">ANDROID APPLICATION PROTOTYPE</div>
                <div class="proto-title">MOBILE AUTHENTICATION &amp; OPERATOR ACCESS (LOGINSCREEN)</div>
            </div>

            <!-- Wireframe Box 1: Mobile Login Screen UI Layout -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        MBPSAAS Mobile Application &mdash; Operator Login Screen (LoginScreen.kt)
                    </div>

                    <!-- App Branding Section -->
                    <div style="text-align:center; padding:12px 0 8px;">
                        <div style="width:60px; height:60px; margin:0 auto 6px; border:2px solid var(--frame-border); border-radius:50%; background:#e8f5e9; display:flex; align-items:center; justify-content:center; font-size:1.7rem;">
                            🐓
                        </div>
                        <div style="font-weight:800; font-size:0.95rem; color:#1b5e20; letter-spacing:1px;">MBPSAAS</div>
                        <div style="font-size:0.68rem; color:var(--mbp-muted); font-weight:600;">Mobile-Based Poultry Security &amp; Alert System</div>
                    </div>

                    <!-- Input Fields -->
                    <div class="wire-form-row">
                        <div class="wire-label">Username or Email:</div>
                        <div class="wire-input">
                            <span class="wire-input-code">user</span>
                            <span class="wire-input-icon">👤</span>
                        </div>
                    </div>

                    <div class="wire-form-row">
                        <div class="wire-label">Password:</div>
                        <div class="wire-input">
                            <span class="wire-input-code">••••••••••••••••</span>
                            <span class="wire-input-icon">👁️</span>
                        </div>
                    </div>

                    <!-- Action Row -->
                    <div class="wire-action-row" style="margin-top:6px;">
                        <span class="wire-badge wire-badge-safe">RETROFIT API READY</span>
                        <div style="display:flex; align-items:center; gap:12px;">
                            <a href="#p2" class="wire-link">Forgot Password?</a>
                            <div class="wire-btn" style="padding:5px 22px; font-size:0.75rem;">Login</div>
                        </div>
                    </div>
                </div>
            </div>

            <div class="wire-arrow-down">&darr;</div>

            <!-- Wireframe Box 2: Forgot Password Step 1 (Account Lookup) -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Mobile Password Recovery &mdash; Step 1: Account Lookup (ForgotPasswordScreen.kt)
                    </div>

                    <div class="wire-tabs">
                        <div class="wire-tab active">Step 1: Who are you?</div>
                        <div class="wire-tab">Step 2: Security Question</div>
                        <div class="wire-tab">Step 3: New Password</div>
                    </div>

                    <div style="font-size:0.72rem; color:var(--mbp-muted); margin-bottom:4px;">
                        Enter your registered username or email to retrieve your security challenge question.
                    </div>

                    <div class="wire-form-row">
                        <div class="wire-label">Username or Email:</div>
                        <div class="wire-input">
                            <span class="wire-input-code">user</span>
                            <span class="wire-input-icon">👤</span>
                        </div>
                    </div>

                    <div class="wire-action-row">
                        <a href="#p1" class="wire-link">Back to Login</a>
                        <div class="wire-btn">Next Step &rarr;</div>
                    </div>
                </div>
            </div>

            <div class="proto-footer">
                <span class="proto-footer-brand">MBPSAAS Android Application Prototype</span>
                <span>Page 1 of 10</span>
            </div>
        </section>


        <!-- ==========================================
             PAGE 2: FORGOT PASSWORD STEP 2 & STEP 3 (NO TOKEN)
             ========================================== -->
        <section class="prototype-page" id="p2">
            <div class="proto-header">
                <div class="proto-tag">ANDROID APPLICATION PROTOTYPE</div>
                <div class="proto-title">PASSWORD RECOVERY CHALLENGE &amp; PASSWORD RESET</div>
            </div>

            <!-- Wireframe Box 1: Step 2 Answer Security Question -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Mobile Password Recovery &mdash; Step 2: Answer Security Question (ExposedDropdownMenuBox)
                    </div>

                    <div class="wire-tabs">
                        <div class="wire-tab">Step 1: Who are you?</div>
                        <div class="wire-tab active">Step 2: Security Question</div>
                        <div class="wire-tab">Step 3: New Password</div>
                    </div>

                    <div class="wire-form-row">
                        <div class="wire-label">Account Verified:</div>
                        <div class="wire-input">
                            <span class="wire-input-code">user</span>
                            <span class="wire-badge wire-badge-safe">VERIFIED</span>
                        </div>
                    </div>

                    <div class="wire-form-row">
                        <div class="wire-label">Security Question:</div>
                        <div class="wire-input">
                            <span class="wire-input-val">What was your first pet's name?</span>
                            <span class="wire-input-icon">▼</span>
                        </div>
                    </div>

                    <!-- Dropdown Options Preview -->
                    <div style="border:1px solid var(--frame-border); background:#ffffff; font-size:0.67rem; margin-left:167px; padding:4px 0; border-radius:2px; box-shadow:0 2px 5px rgba(0,0,0,0.1);">
                        <div style="padding:4px 10px; background:#e8f5e9; font-weight:700;">✓ What was your first pet's name?</div>
                        <div style="padding:4px 10px; color:#546e5a;">What is your mother's maiden name?</div>
                        <div style="padding:4px 10px; color:#546e5a;">What was the name of your elementary school?</div>
                        <div style="padding:4px 10px; color:#546e5a;">What city were you born in?</div>
                    </div>

                    <div class="wire-form-row">
                        <div class="wire-label">Your Answer:</div>
                        <div class="wire-input">
                            <span class="wire-input-code">Bantay</span>
                        </div>
                    </div>

                    <div class="wire-action-row">
                        <a href="#p1" class="wire-link">Back to Login</a>
                        <div class="wire-btn wire-btn-success">Verify Answer &rarr;</div>
                    </div>
                </div>
            </div>

            <div class="wire-arrow-down">&darr;</div>

            <!-- Wireframe Box 2: Step 3 Choose New Password (Token Removed) & Success Dialog -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Mobile Password Recovery &mdash; Step 3: Choose New Password &amp; Confirmation Dialog
                    </div>

                    <div class="wire-tabs">
                        <div class="wire-tab">Step 1: Who are you?</div>
                        <div class="wire-tab">Step 2: Security Question</div>
                        <div class="wire-tab active">Step 3: New Password</div>
                    </div>

                    <div class="wire-form-row">
                        <div class="wire-label">New Password:</div>
                        <div class="wire-input">
                            <span class="wire-input-code">••••••••••••••••</span>
                            <span class="wire-input-icon">👁️</span>
                        </div>
                    </div>

                    <div class="wire-form-row">
                        <div class="wire-label">Confirm New Password:</div>
                        <div class="wire-input">
                            <span class="wire-input-code">••••••••••••••••</span>
                            <span class="wire-input-icon">👁️</span>
                        </div>
                    </div>

                    <!-- Material 3 Confirmation Modal Box Inside -->
                    <div style="border:1.5px solid var(--frame-border); background:#ffffff; border-radius:6px; padding:10px 14px; margin-top:4px;">
                        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:4px;">
                            <span style="font-weight:800; font-size:0.75rem; color:#1b5e20;">Password Updated (AlertDialog)</span>
                            <span class="wire-badge wire-badge-safe">RESET SUCCESSFUL</span>
                        </div>
                        <div style="font-size:0.68rem; color:#27382b; line-height:1.4;">
                            Your password has been reset successfully. You can now log in with your new password.
                        </div>
                    </div>

                    <div class="wire-action-row">
                        <a href="#p1" class="wire-link">Back to Login Screen</a>
                        <div class="wire-btn wire-btn-success">Reset Password &amp; Continue</div>
                    </div>
                </div>
            </div>

            <div class="proto-footer">
                <span class="proto-footer-brand">MBPSAAS Android Application Prototype</span>
                <span>Page 2 of 10</span>
            </div>
        </section>


        <!-- ==========================================
             PAGE 3: MOBILE DASHBOARD (NORMAL SAFE STATE)
             ========================================== -->
        <section class="prototype-page" id="p3">
            <div class="proto-header">
                <div class="proto-tag">ANDROID APPLICATION PROTOTYPE</div>
                <div class="proto-title">DASHBOARD SCREEN &mdash; NORMAL SAFE STATE (DASHBOARDSCREEN)</div>
            </div>

            <!-- Wireframe Box 1: MotionStatusCard Safe State -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Overall Farm Security Banner &mdash; MotionStatusCard (Safe State)
                    </div>

                    <!-- MotionStatusCard (Safe State) Mockup -->
                    <div style="border:1.5px solid #2e7d32; background:#e8f5e9; border-radius:12px; padding:16px; display:flex; align-items:center; gap:16px;">
                        <div style="width:52px; height:52px; background:#2e7d32; border-radius:50%; display:flex; align-items:center; justify-content:center; color:#ffffff; font-size:1.6rem; flex-shrink:0;">
                            🛡️
                        </div>
                        <div style="flex:1;">
                            <div style="font-weight:800; font-size:0.88rem; color:#1b5e20; letter-spacing:0.5px;">
                                ALL POULTRY ZONES SAFE
                            </div>
                            <div style="font-size:0.7rem; color:#2e7d32; margin-top:2px;">
                                System Online &mdash; Monitoring active
                            </div>
                        </div>
                        <span class="wire-badge wire-badge-safe">PERIMETER CLEAR</span>
                    </div>

                    <div style="font-size:0.67rem; color:var(--mbp-muted); line-height:1.45;">
                        <strong>Code Logic:</strong> <code>isMotionDetected = overallStatus == "MOTION_DETECTED" || latestEvent?.eventType == "MOTION_DETECTED" || latestEvent?.buzzerTriggered == true</code>. When <code>false</code>, uses <code>SafeColor (#2E7D32)</code> and primaryContainer.
                    </div>
                </div>
            </div>

            <div class="wire-arrow-down">&darr;</div>

            <!-- Wireframe Box 2: ZoneStatusGrid Safe State -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Poultry Farm Zone Status Grid &mdash; ZoneStatusGrid (3-Zone Triplet)
                    </div>

                    <div style="font-size:0.72rem; font-weight:700; color:#1b5e20;">
                        Poultry Farm Zones (Real-Time Sensor Telemetry)
                    </div>

                    <!-- 3-Column Card Row Simulation -->
                    <div style="display:grid; grid-template-columns:repeat(3, 1fr); gap:8px;">
                        <!-- Zone A -->
                        <div style="border:1px solid var(--frame-border); background:#f4fbf5; border-radius:8px; padding:10px;">
                            <div style="display:flex; justify-content:space-between; align-items:center;">
                                <div style="width:8px; height:8px; background:#2e7d32; border-radius:50%;"></div>
                                <span style="font-size:0.85rem; color:#2e7d32;">✓</span>
                            </div>
                            <div style="font-weight:700; font-size:0.75rem; color:#102a16; margin-top:8px;">Coop Zone A</div>
                            <div style="font-size:0.65rem; font-weight:700; color:#2e7d32;">Clear</div>
                            <div style="font-size:0.58rem; color:var(--mbp-muted); margin-top:2px;">Brooder House</div>
                        </div>

                        <!-- Zone B -->
                        <div style="border:1px solid var(--frame-border); background:#f4fbf5; border-radius:8px; padding:10px;">
                            <div style="display:flex; justify-content:space-between; align-items:center;">
                                <div style="width:8px; height:8px; background:#2e7d32; border-radius:50%;"></div>
                                <span style="font-size:0.85rem; color:#2e7d32;">✓</span>
                            </div>
                            <div style="font-weight:700; font-size:0.75rem; color:#102a16; margin-top:8px;">Coop Zone B</div>
                            <div style="font-size:0.65rem; font-weight:700; color:#2e7d32;">Clear</div>
                            <div style="font-size:0.58rem; color:var(--mbp-muted); margin-top:2px;">Layer Coop</div>
                        </div>

                        <!-- Zone C -->
                        <div style="border:1px solid var(--frame-border); background:#f4fbf5; border-radius:8px; padding:10px;">
                            <div style="display:flex; justify-content:space-between; align-items:center;">
                                <div style="width:8px; height:8px; background:#2e7d32; border-radius:50%;"></div>
                                <span style="font-size:0.85rem; color:#2e7d32;">✓</span>
                            </div>
                            <div style="font-weight:700; font-size:0.75rem; color:#102a16; margin-top:8px;">Coop Zone C</div>
                            <div style="font-size:0.65rem; font-weight:700; color:#2e7d32;">Clear</div>
                            <div style="font-size:0.58rem; color:var(--mbp-muted); margin-top:2px;">Free-Range Pen</div>
                        </div>
                    </div>

                    <div class="wire-action-row">
                        <span style="font-size:0.65rem; color:var(--mbp-muted);">Mapping helper: getZoneDisplayName("ROOMA") &rarr; "Coop Zone A"</span>
                        <span class="wire-badge wire-badge-safe">ALL ZONES SECURED</span>
                    </div>
                </div>
            </div>

            <div class="proto-footer">
                <span class="proto-footer-brand">MBPSAAS Android Application Prototype</span>
                <span>Page 3 of 10</span>
            </div>
        </section>


        <!-- ==========================================
             PAGE 4: MOBILE DASHBOARD (INTRUSION ALERT STATE)
             ========================================== -->
        <section class="prototype-page" id="p4">
            <div class="proto-header">
                <div class="proto-tag">ANDROID APPLICATION PROTOTYPE</div>
                <div class="proto-title">DASHBOARD SCREEN &mdash; INTRUSION ALERT STATE (DASHBOARDSCREEN)</div>
            </div>

            <!-- Wireframe Box 1: MotionStatusCard Alert State -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Critical Intrusion Alert Banner &mdash; MotionStatusCard (Alarm Active)
                    </div>

                    <!-- MotionStatusCard (Alert State) Mockup -->
                    <div style="border:2px solid #c62828; background:#ffebee; border-radius:12px; padding:16px; display:flex; align-items:center; gap:16px;">
                        <div style="width:52px; height:52px; background:#c62828; border-radius:50%; display:flex; align-items:center; justify-content:center; color:#ffffff; font-size:1.6rem; flex-shrink:0;">
                            🚨
                        </div>
                        <div style="flex:1;">
                            <div style="font-weight:800; font-size:0.88rem; color:#c62828; letter-spacing:0.5px;">
                                INTRUSION ALERT &mdash; Coop Zone A
                            </div>
                            <div style="font-size:0.7rem; color:#8e0000; margin-top:2px;">
                                Last activity: 02:14:18 (Coop Zone A) &bull; Siren Buzzer Active
                            </div>
                        </div>
                        <span class="wire-badge wire-badge-alert">BREACH DETECTED</span>
                    </div>

                    <div style="font-size:0.67rem; color:var(--mbp-muted); line-height:1.45;">
                        <strong>Code Logic:</strong> Triggered when <code>overallStatus == "MOTION_DETECTED"</code> or <code>latestEvent?.buzzerTriggered == true</code>. Card color changes dynamically to <code>errorContainer</code> with <code>AlertColor (#C62828)</code>.
                    </div>
                </div>
            </div>

            <div class="wire-arrow-down">&darr;</div>

            <!-- Wireframe Box 2: ZoneStatusGrid Triggered State -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Poultry Farm Zone Status Grid &mdash; Breach Indicator (Coop Zone A Triggered)
                    </div>

                    <div style="font-size:0.72rem; font-weight:700; color:#1b5e20;">
                        Poultry Farm Zones (Dynamic Visual Recomposition)
                    </div>

                    <!-- 3-Column Card Row Simulation with Zone A Triggered -->
                    <div style="display:grid; grid-template-columns:repeat(3, 1fr); gap:8px;">
                        <!-- Zone A (Triggered) -->
                        <div style="border:2px solid #c62828; background:#ffebee; border-radius:8px; padding:10px;">
                            <div style="display:flex; justify-content:space-between; align-items:center;">
                                <div style="width:8px; height:8px; background:#c62828; border-radius:50%;"></div>
                                <span style="font-size:0.85rem; color:#c62828;">⚠️</span>
                            </div>
                            <div style="font-weight:800; font-size:0.75rem; color:#c62828; margin-top:8px;">Coop Zone A</div>
                            <div style="font-size:0.68rem; font-weight:800; color:#c62828;">MOTION</div>
                            <div style="font-size:0.58rem; color:#8e0000; margin-top:2px;">Brooder House (PIR 1)</div>
                        </div>

                        <!-- Zone B (Clear) -->
                        <div style="border:1px solid var(--frame-border); background:#f4fbf5; border-radius:8px; padding:10px;">
                            <div style="display:flex; justify-content:space-between; align-items:center;">
                                <div style="width:8px; height:8px; background:#2e7d32; border-radius:50%;"></div>
                                <span style="font-size:0.85rem; color:#2e7d32;">✓</span>
                            </div>
                            <div style="font-weight:700; font-size:0.75rem; color:#102a16; margin-top:8px;">Coop Zone B</div>
                            <div style="font-size:0.65rem; font-weight:700; color:#2e7d32;">Clear</div>
                            <div style="font-size:0.58rem; color:var(--mbp-muted); margin-top:2px;">Layer Coop (PIR 2)</div>
                        </div>

                        <!-- Zone C (Clear) -->
                        <div style="border:1px solid var(--frame-border); background:#f4fbf5; border-radius:8px; padding:10px;">
                            <div style="display:flex; justify-content:space-between; align-items:center;">
                                <div style="width:8px; height:8px; background:#2e7d32; border-radius:50%;"></div>
                                <span style="font-size:0.85rem; color:#2e7d32;">✓</span>
                            </div>
                            <div style="font-weight:700; font-size:0.75rem; color:#102a16; margin-top:8px;">Coop Zone C</div>
                            <div style="font-size:0.65rem; font-weight:700; color:#2e7d32;">Clear</div>
                            <div style="font-size:0.58rem; color:var(--mbp-muted); margin-top:2px;">Free-Range (PIR 3)</div>
                        </div>
                    </div>

                    <div class="wire-action-row">
                        <span style="font-size:0.65rem; color:#c62828; font-weight:700;">🚨 Automated GSM SMS dispatched to Farm Admin</span>
                        <div class="wire-btn wire-btn-danger wire-btn-sm">View Breach Details</div>
                    </div>
                </div>
            </div>

            <div class="proto-footer">
                <span class="proto-footer-brand">MBPSAAS Android Application Prototype</span>
                <span>Page 4 of 10</span>
            </div>
        </section>


        <!-- ==========================================
             PAGE 5: PIR SENSOR CONTROLS & ZONE SWITCHES
             ========================================== -->
        <section class="prototype-page" id="p5">
            <div class="proto-header">
                <div class="proto-tag">ANDROID APPLICATION PROTOTYPE</div>
                <div class="proto-title">PIR SENSORS ARMED / BYPASS CONTROLS (SENSORCONTROLCARD)</div>
            </div>

            <!-- Wireframe Box 1: SensorControlCard UI Mockup -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Sensor Control Card &mdash; Interactive Multi-Zone Switches (SensorControlCard.kt)
                    </div>

                    <div style="border:1px solid var(--frame-border); background:#f4fbf5; border-radius:12px; padding:14px;">
                        <div style="display:flex; align-items:center; gap:8px; margin-bottom:12px;">
                            <span style="font-size:1.1rem; color:#1b5e20;">📡</span>
                            <span style="font-weight:800; font-size:0.82rem; color:#1b5e20;">PIR Sensors Control</span>
                        </div>

                        <!-- Sensor Item 1: Coop Zone A -->
                        <div style="display:flex; justify-content:space-between; align-items:center; padding:6px 0;">
                            <div>
                                <div style="font-weight:700; font-size:0.75rem; color:#102a16;">Coop Zone A (Brooder House)</div>
                                <div style="font-size:0.62rem; color:var(--mbp-muted);">PIR Sensor 1 (Pin 12) &bull; Active &bull; Sounding Enabled</div>
                            </div>
                            <div style="display:flex; align-items:center; gap:8px;">
                                <span class="wire-badge wire-badge-safe">ARMED</span>
                                <!-- Switch ON -->
                                <div style="width:36px; height:20px; background:#2e7d32; border-radius:10px; position:relative; cursor:pointer;">
                                    <div style="width:16px; height:16px; background:#ffffff; border-radius:50%; position:absolute; top:2px; right:2px;"></div>
                                </div>
                            </div>
                        </div>
                        <div style="border-top:1px solid var(--mbp-border-light); margin:4px 0;"></div>

                        <!-- Sensor Item 2: Coop Zone B -->
                        <div style="display:flex; justify-content:space-between; align-items:center; padding:6px 0;">
                            <div>
                                <div style="font-weight:700; font-size:0.75rem; color:#102a16;">Coop Zone B (Layer Coop)</div>
                                <div style="font-size:0.62rem; color:var(--mbp-muted);">PIR Sensor 2 (Pin 14) &bull; Active &bull; Sounding Enabled</div>
                            </div>
                            <div style="display:flex; align-items:center; gap:8px;">
                                <span class="wire-badge wire-badge-safe">ARMED</span>
                                <!-- Switch ON -->
                                <div style="width:36px; height:20px; background:#2e7d32; border-radius:10px; position:relative; cursor:pointer;">
                                    <div style="width:16px; height:16px; background:#ffffff; border-radius:50%; position:absolute; top:2px; right:2px;"></div>
                                </div>
                            </div>
                        </div>
                        <div style="border-top:1px solid var(--mbp-border-light); margin:4px 0;"></div>

                        <!-- Sensor Item 3: Coop Zone C -->
                        <div style="display:flex; justify-content:space-between; align-items:center; padding:6px 0;">
                            <div>
                                <div style="font-weight:700; font-size:0.75rem; color:#102a16;">Coop Zone C (Free-Range Pen)</div>
                                <div style="font-size:0.62rem; color:var(--mbp-muted);">PIR Sensor 3 (Pin 13) &bull; Bypassed for Maintenance</div>
                            </div>
                            <div style="display:flex; align-items:center; gap:8px;">
                                <span class="wire-badge" style="background:#f1f5f9; color:#64748b;">BYPASSED</span>
                                <!-- Switch OFF -->
                                <div style="width:36px; height:20px; background:#94a3b8; border-radius:10px; position:relative; cursor:pointer;">
                                    <div style="width:16px; height:16px; background:#ffffff; border-radius:50%; position:absolute; top:2px; left:2px;"></div>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <div class="wire-arrow-down">&darr;</div>

            <!-- Wireframe Box 2: State Synchronization & Optimistic UI Dispatch -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Sensor State Synchronization &amp; Backend API Dispatch Pipeline
                    </div>

                    <div style="display:flex; flex-direction:column; gap:6px; font-size:0.7rem;">
                        <div style="padding:6px 8px; background:#ffffff; border:1px solid var(--frame-border); display:flex; justify-content:space-between; align-items:center;">
                            <span><strong>1. User Action:</strong> Operator toggles Switch in Compose UI</span>
                            <span class="wire-badge wire-badge-safe">INSTANT UI TOGGLE</span>
                        </div>
                        <div style="padding:6px 8px; background:#ffffff; border:1px solid var(--frame-border); display:flex; justify-content:space-between; align-items:center;">
                            <span><strong>2. Optimistic Update:</strong> <code>sensors = sensors.map { if (code == it.sensorCode) it.copy(isEnabled = new) }</code></span>
                            <span class="wire-badge wire-badge-safe">ZERO UI LAG</span>
                        </div>
                        <div style="padding:6px 8px; background:#ffffff; border:1px solid var(--frame-border); display:flex; justify-content:space-between; align-items:center;">
                            <span><strong>3. HTTP Dispatch:</strong> <code>ApiClient.service.updateSensorStatus(code, isEnabled)</code></span>
                            <span class="wire-badge wire-badge-blue">RETROFIT POST</span>
                        </div>
                        <div style="padding:6px 8px; background:#ffffff; border:1px solid var(--frame-border); display:flex; justify-content:space-between; align-items:center;">
                            <span><strong>4. Rollback on Failure:</strong> If exception occurs, state safely reverts to previous server value</span>
                            <span class="wire-badge wire-badge-alert">AUTOMATIC ROLLBACK</span>
                        </div>
                    </div>

                    <div class="wire-action-row">
                        <span style="font-size:0.65rem; color:var(--mbp-muted);">Endpoint: /mbpsaas_api/update_sensor_status.php</span>
                        <div class="wire-btn wire-btn-secondary wire-btn-sm">Arm All Sensors</div>
                    </div>
                </div>
            </div>

            <div class="proto-footer">
                <span class="proto-footer-brand">MBPSAAS Android Application Prototype</span>
                <span>Page 5 of 10</span>
            </div>
        </section>


        <!-- ==========================================
             PAGE 6: RECENT INTRUSION ACTIVITY FEED
             ========================================== -->
        <section class="prototype-page" id="p6">
            <div class="proto-header">
                <div class="proto-tag">ANDROID APPLICATION PROTOTYPE</div>
                <div class="proto-title">DASHBOARD SCREEN &mdash; RECENT INTRUSION ACTIVITY FEED</div>
            </div>

            <!-- Wireframe Box 1: Recent Intrusion Activity Section -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Recent Intrusion Activity Feed Component (DashboardScreen.kt)
                    </div>

                    <div style="font-weight:700; font-size:0.75rem; color:#1b5e20; margin-bottom:4px;">
                        Recent Intrusion Activity
                    </div>

                    <!-- Feed List -->
                    <div style="display:flex; flex-direction:column; gap:6px;">
                        <!-- Row 1 -->
                        <div style="border:1px solid var(--frame-border); background:#ffffff; padding:6px 10px; border-radius:6px; display:flex; justify-content:space-between; align-items:center;">
                            <div style="display:flex; align-items:center; gap:8px;">
                                <div style="width:28px; height:28px; background:#ffebee; border-radius:50%; display:flex; align-items:center; justify-content:center; color:#c62828; font-size:0.8rem;">🚨</div>
                                <div>
                                    <div style="font-weight:700; font-size:0.72rem; color:#102a16;">Coop Zone A (Brooder House)</div>
                                    <div style="font-size:0.62rem; color:var(--mbp-muted);">2026-09-24 02:14:18 &bull; Siren Buzzer Active</div>
                                </div>
                            </div>
                            <span class="wire-badge wire-badge-alert">MOTION DETECTED</span>
                        </div>

                        <!-- Row 2 -->
                        <div style="border:1px solid var(--frame-border); background:#ffffff; padding:6px 10px; border-radius:6px; display:flex; justify-content:space-between; align-items:center;">
                            <div style="display:flex; align-items:center; gap:8px;">
                                <div style="width:28px; height:28px; background:#e8f5e9; border-radius:50%; display:flex; align-items:center; justify-content:center; color:#2e7d32; font-size:0.8rem;">🛡️</div>
                                <div>
                                    <div style="font-weight:700; font-size:0.72rem; color:#102a16;">Coop Zone A (Brooder House)</div>
                                    <div style="font-size:0.62rem; color:var(--mbp-muted);">2026-09-24 02:14:45 &bull; Hold 2000ms Cleared</div>
                                </div>
                            </div>
                            <span class="wire-badge wire-badge-safe">MOTION STOPPED</span>
                        </div>

                        <!-- Row 3 -->
                        <div style="border:1px solid var(--frame-border); background:#ffffff; padding:6px 10px; border-radius:6px; display:flex; justify-content:space-between; align-items:center;">
                            <div style="display:flex; align-items:center; gap:8px;">
                                <div style="width:28px; height:28px; background:#ffebee; border-radius:50%; display:flex; align-items:center; justify-content:center; color:#c62828; font-size:0.8rem;">🚨</div>
                                <div>
                                    <div style="font-weight:700; font-size:0.72rem; color:#102a16;">Coop Zone C (Free-Range Pen)</div>
                                    <div style="font-size:0.62rem; color:var(--mbp-muted);">2026-09-24 01:42:10 &bull; Siren Buzzer Active</div>
                                </div>
                            </div>
                            <span class="wire-badge wire-badge-alert">MOTION DETECTED</span>
                        </div>
                    </div>

                    <div class="wire-action-row">
                        <span style="font-size:0.65rem; color:var(--mbp-muted);">Rendered via LazyColumn take(10) items</span>
                        <a href="#p7" class="wire-link">View All Triggered Alerts &rarr;</a>
                    </div>
                </div>
            </div>

            <div class="wire-arrow-down">&darr;</div>

            <!-- Wireframe Box 2: MotionEventRow Component Anatomy -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        MotionEventRow Component Anatomy &amp; Data Model Breakdown
                    </div>

                    <table class="wire-table">
                        <thead>
                            <tr>
                                <th>Model Field</th>
                                <th>Kotlin Data Type</th>
                                <th>Sample Value</th>
                                <th>UI Rendering Function</th>
                            </tr>
                        </thead>
                        <tbody>
                            <tr>
                                <td class="wire-input-code">zone / zoneLabel</td>
                                <td>String / String?</td>
                                <td class="wire-input-code">"ROOMA" / "Coop Zone A"</td>
                                <td class="text-left">Displays bold title via <code>getZoneDisplayName()</code></td>
                            </tr>
                            <tr>
                                <td class="wire-input-code">eventType</td>
                                <td>String</td>
                                <td class="wire-input-code">"MOTION_DETECTED"</td>
                                <td class="text-left">Selects Red Alert vs Green Safe pill badge</td>
                            </tr>
                            <tr>
                                <td class="wire-input-code">detectedAt</td>
                                <td>String</td>
                                <td class="wire-input-code">"2026-09-24 02:14:18"</td>
                                <td class="text-left">Formats relative time &bull; <code>bodySmall</code> caption</td>
                            </tr>
                            <tr>
                                <td class="wire-input-code">buzzerTriggered</td>
                                <td>Boolean</td>
                                <td class="wire-input-code">true</td>
                                <td class="text-left">Displays acoustic alarm badge &amp; sounds mobile audio</td>
                            </tr>
                        </tbody>
                    </table>

                    <div style="font-size:0.67rem; background:#f4fbf5; padding:6px 8px; border:1px solid var(--mbp-border-light);">
                        💡 <strong>Performance Note:</strong> Rendered with Compose <code>key(event.id)</code> inside <code>LazyColumn</code> to guarantee smooth 60fps scrolling without unnecessary item recomposition during live 3s polling cycles.
                    </div>
                </div>
            </div>

            <div class="proto-footer">
                <span class="proto-footer-brand">MBPSAAS Android Application Prototype</span>
                <span>Page 6 of 10</span>
            </div>
        </section>


        <!-- ==========================================
             PAGE 7: TRIGGERED ALERTS MODULE (DATE & ZONE FILTERS)
             ========================================== -->
        <section class="prototype-page" id="p7">
            <div class="proto-header">
                <div class="proto-tag">ANDROID APPLICATION PROTOTYPE</div>
                <div class="proto-title">TRIGGERED ALERTS &mdash; DATE &amp; ZONE FILTERING (TRIGGEREDALERTSSCREEN)</div>
            </div>

            <!-- Wireframe Box 1: Alert Header Card & Date Filter Chips -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Triggered Alerts Screen &mdash; Header Card &amp; Date Filter Chips
                    </div>

                    <!-- Header Title Card -->
                    <div style="border:1.5px solid #c62828; background:#ffebee; border-radius:12px; padding:12px 14px; display:flex; align-items:center; gap:12px;">
                        <div style="width:40px; height:40px; background:#c62828; border-radius:50%; display:flex; align-items:center; justify-content:center; color:#fff; font-size:1.3rem;">
                            🔔
                        </div>
                        <div>
                            <div style="font-weight:800; font-size:0.85rem; color:#c62828;">Triggered Intrusion Alerts</div>
                            <div style="font-size:0.68rem; color:#8e0000;">14 alert logs recorded &bull; Off-Hours Perimeter Breaches</div>
                        </div>
                    </div>

                    <!-- Date Filter Chips -->
                    <div style="margin-top:4px;">
                        <div style="font-size:0.7rem; font-weight:700; color:#1b5e20; margin-bottom:4px;">Filter Alerts by Date:</div>
                        <div style="display:flex; gap:6px;">
                            <div class="wire-btn wire-btn-secondary wire-btn-sm">All Dates</div>
                            <div class="wire-btn wire-btn-sm" style="background:#1b5e20;">Today (Active)</div>
                            <div class="wire-btn wire-btn-secondary wire-btn-sm">Yesterday</div>
                            <div class="wire-btn wire-btn-secondary wire-btn-sm">📅 Custom Date...</div>
                        </div>
                    </div>

                    <!-- Zone Filter Chips -->
                    <div>
                        <div style="font-size:0.7rem; font-weight:700; color:#1b5e20; margin-bottom:4px;">Filter Alerts by Zone:</div>
                        <div style="display:flex; gap:6px;">
                            <div class="wire-btn wire-btn-sm">All Zones (14)</div>
                            <div class="wire-btn wire-btn-secondary wire-btn-sm">Coop Zone A (8)</div>
                            <div class="wire-btn wire-btn-secondary wire-btn-sm">Coop Zone B (2)</div>
                            <div class="wire-btn wire-btn-secondary wire-btn-sm">Coop Zone C (4)</div>
                        </div>
                    </div>
                </div>
            </div>

            <div class="wire-arrow-down">&darr;</div>

            <!-- Wireframe Box 2: Native Android DatePickerDialog Integration -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Android Native DatePickerDialog &amp; Custom Date Filtering Flow
                    </div>

                    <!-- DatePickerDialog Simulation -->
                    <div style="border:1.5px solid var(--frame-border); background:#ffffff; border-radius:8px; padding:12px 16px; max-width:320px; margin:4px auto; box-shadow:0 3px 10px rgba(0,0,0,0.12);">
                        <div style="font-size:0.62rem; color:var(--mbp-muted); text-transform:uppercase; font-weight:700;">Select Date</div>
                        <div style="font-size:1.1rem; font-weight:800; color:#1b5e20; margin-bottom:8px;">Thu, Sep 24, 2026</div>

                        <!-- Mini Calendar Grid -->
                        <div style="display:grid; grid-template-columns:repeat(7, 1fr); gap:4px; text-align:center; font-size:0.6rem; margin-bottom:8px;">
                            <span style="font-weight:700; color:var(--mbp-muted);">S</span>
                            <span style="font-weight:700; color:var(--mbp-muted);">M</span>
                            <span style="font-weight:700; color:var(--mbp-muted);">T</span>
                            <span style="font-weight:700; color:var(--mbp-muted);">W</span>
                            <span style="font-weight:700; color:var(--mbp-muted);">T</span>
                            <span style="font-weight:700; color:var(--mbp-muted);">F</span>
                            <span style="font-weight:700; color:var(--mbp-muted);">S</span>
                            <span>20</span><span>21</span><span>22</span><span>23</span>
                            <span style="background:#2e7d32; color:#fff; border-radius:50%; font-weight:700;">24</span>
                            <span>25</span><span>26</span>
                        </div>

                        <div style="display:flex; justify-content:flex-end; gap:8px;">
                            <div class="wire-btn wire-btn-secondary wire-btn-sm">Cancel</div>
                            <div class="wire-btn wire-btn-success wire-btn-sm">OK</div>
                        </div>
                    </div>

                    <div style="font-size:0.67rem; color:var(--mbp-muted); line-height:1.45;">
                        <strong>Implementation:</strong> Formats selected date into ISO prefix <code>selectedCustomDate = SimpleDateFormat("yyyy-MM-dd").format(cal.time)</code>. The Compose <code>remember(events, selectedDateFilter, selectedZoneFilter)</code> filter efficiently prunes the list without recomposing unaffected screens.
                    </div>
                </div>
            </div>

            <div class="proto-footer">
                <span class="proto-footer-brand">MBPSAAS Android Application Prototype</span>
                <span>Page 7 of 10</span>
            </div>
        </section>


        <!-- ==========================================
             PAGE 8: TRIGGERED ALERTS FEED & EMPTY STATES
             ========================================== -->
        <section class="prototype-page" id="p8">
            <div class="proto-header">
                <div class="proto-tag">ANDROID APPLICATION PROTOTYPE</div>
                <div class="proto-title">TRIGGERED ALERTS FEED &amp; EMPTY VIEW STATES</div>
            </div>

            <!-- Wireframe Box 1: Filtered Alerts Stream -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Filtered Intrusion Alerts Feed &mdash; Chronological Alert Cards
                    </div>

                    <div style="display:flex; flex-direction:column; gap:6px;">
                        <!-- Alert Item 1 -->
                        <div style="border:1.5px solid #c62828; background:#fff; border-radius:8px; padding:8px 12px; display:flex; justify-content:space-between; align-items:center;">
                            <div style="display:flex; align-items:center; gap:10px;">
                                <div style="width:32px; height:32px; background:#ffebee; border-radius:50%; display:flex; align-items:center; justify-content:center; color:#c62828; font-size:1rem;">🚨</div>
                                <div>
                                    <div style="font-weight:800; font-size:0.75rem; color:#c62828;">Coop Zone A (Brooder House)</div>
                                    <div style="font-size:0.62rem; color:var(--mbp-muted);">2026-09-24 02:14:18 &bull; Continuous trigger 27s</div>
                                </div>
                            </div>
                            <div style="text-align:right;">
                                <span class="wire-badge wire-badge-alert">ALERT TRIGGERED</span>
                                <div style="font-size:0.58rem; color:#c62828; font-weight:700; margin-top:2px;">Buzzer Active</div>
                            </div>
                        </div>

                        <!-- Alert Item 2 -->
                        <div style="border:1.5px solid #c62828; background:#fff; border-radius:8px; padding:8px 12px; display:flex; justify-content:space-between; align-items:center;">
                            <div style="display:flex; align-items:center; gap:10px;">
                                <div style="width:32px; height:32px; background:#ffebee; border-radius:50%; display:flex; align-items:center; justify-content:center; color:#c62828; font-size:1rem;">🚨</div>
                                <div>
                                    <div style="font-weight:800; font-size:0.75rem; color:#c62828;">Coop Zone C (Free-Range Pen)</div>
                                    <div style="font-size:0.62rem; color:var(--mbp-muted);">2026-09-24 01:42:10 &bull; Continuous trigger 19s</div>
                                </div>
                            </div>
                            <div style="text-align:right;">
                                <span class="wire-badge wire-badge-alert">ALERT TRIGGERED</span>
                                <div style="font-size:0.58rem; color:#c62828; font-weight:700; margin-top:2px;">Buzzer Active</div>
                            </div>
                        </div>
                    </div>

                    <div class="wire-action-row">
                        <span style="font-size:0.65rem; color:var(--mbp-muted);">Filtered only events where eventType == "MOTION_DETECTED"</span>
                        <span class="wire-badge wire-badge-safe">DELIVERED TO GSM</span>
                    </div>
                </div>
            </div>

            <div class="wire-arrow-down">&darr;</div>

            <!-- Wireframe Box 2: Empty / Zero Alerts State -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Empty Filter &amp; Zero Intrusion Results View State
                    </div>

                    <!-- Empty State Viewport -->
                    <div style="border:1px dashed var(--frame-border); background:#f4fbf5; border-radius:8px; padding:24px; text-align:center;">
                        <div style="font-size:2rem; margin-bottom:6px;">🛡️</div>
                        <div style="font-weight:800; font-size:0.85rem; color:#1b5e20;">No Intrusion Alerts Found</div>
                        <div style="font-size:0.7rem; color:var(--mbp-muted); max-width:320px; margin:4px auto 12px;">
                            There are no recorded intrusion alerts matching the selected date and zone filter. Your poultry farm is secure.
                        </div>
                        <div class="wire-btn wire-btn-secondary wire-btn-sm">Reset Date Filter to All Dates</div>
                    </div>

                    <div style="font-size:0.67rem; color:var(--mbp-muted); line-height:1.45;">
                        <strong>Loading State Note:</strong> When <code>isLoading == true</code>, a centered <code>CircularProgressIndicator()</code> is rendered in place of the list to provide responsive visual feedback during data fetching.
                    </div>
                </div>
            </div>

            <div class="proto-footer">
                <span class="proto-footer-brand">MBPSAAS Android Application Prototype</span>
                <span>Page 8 of 10</span>
            </div>
        </section>


        <!-- ==========================================
             PAGE 9: ACTIVITY LOG & OPERATOR PROFILE (USER)
             ========================================== -->
        <section class="prototype-page" id="p9">
            <div class="proto-header">
                <div class="proto-tag">ANDROID APPLICATION PROTOTYPE</div>
                <div class="proto-title">ACTIVITY LOG STREAM &amp; OPERATOR PROFILE MANAGEMENT</div>
            </div>

            <!-- Wireframe Box 1: ActivityLogScreen Stream -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Activity Log Screen &mdash; Full Telemetry Stream (ActivityLogScreen.kt)
                    </div>

                    <div style="display:flex; flex-direction:column; gap:5px;">
                        <div style="border:1px solid var(--frame-border); padding:5px 8px; background:#ffffff; border-radius:4px; display:flex; justify-content:space-between; align-items:center; font-size:0.68rem;">
                            <span><strong>ROOMA:</strong> Coop Zone A &bull; MOTION_DETECTED</span>
                            <span class="wire-input-code">2026-09-24 02:14:18</span>
                        </div>
                        <div style="border:1px solid var(--frame-border); padding:5px 8px; background:#ffffff; border-radius:4px; display:flex; justify-content:space-between; align-items:center; font-size:0.68rem;">
                            <span><strong>ROOMA:</strong> Coop Zone A &bull; MOTION_STOPPED</span>
                            <span class="wire-input-code">2026-09-24 02:14:45</span>
                        </div>
                        <div style="border:1px solid var(--frame-border); padding:5px 8px; background:#ffffff; border-radius:4px; display:flex; justify-content:space-between; align-items:center; font-size:0.68rem;">
                            <span><strong>ROOMC:</strong> Coop Zone C &bull; MOTION_DETECTED</span>
                            <span class="wire-input-code">2026-09-24 01:42:10</span>
                        </div>
                    </div>

                    <div class="wire-action-row">
                        <span style="font-size:0.65rem; color:var(--mbp-muted);">Comprehensive audit record of all detection and clear transitions</span>
                        <span class="wire-badge wire-badge-safe">LOGS SYNCHRONIZED</span>
                    </div>
                </div>
            </div>

            <div class="wire-arrow-down">&darr;</div>

            <!-- Wireframe Box 2: ProfileScreen Operator Credentials (User) -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Operator Profile &mdash; ProfileScreen.kt (View &amp; Edit In-Place)
                    </div>

                    <!-- Profile Avatar Card -->
                    <div style="border:1px solid var(--frame-border); background:#ffffff; border-radius:8px; padding:10px 14px; display:flex; align-items:center; gap:12px;">
                        <div style="width:48px; height:48px; background:#e8f5e9; border:2px solid #2e7d32; border-radius:50%; display:flex; align-items:center; justify-content:center; font-size:1.4rem;">
                            👤
                        </div>
                        <div style="flex:1;">
                            <div style="font-weight:800; font-size:0.85rem; color:#1b5e20;">User</div>
                            <div style="font-size:0.65rem; color:var(--mbp-muted);">Farm Operator &bull; user@poultryfarm.com</div>
                        </div>
                        <div class="wire-btn wire-btn-secondary wire-btn-sm">Edit Profile</div>
                    </div>

                    <!-- Edit Form Fields Preview -->
                    <div class="wire-form-row">
                        <div class="wire-label">Username:</div>
                        <div class="wire-input"><span class="wire-input-code">user</span></div>
                    </div>
                    <div class="wire-form-row">
                        <div class="wire-label">Email:</div>
                        <div class="wire-input"><span class="wire-input-code">user@poultryfarm.com</span></div>
                    </div>
                    <div class="wire-form-row">
                        <div class="wire-label">Current Password:</div>
                        <div class="wire-input"><span class="wire-input-code">••••••••••••••••</span></div>
                    </div>
                    <div class="wire-form-row">
                        <div class="wire-label">New Password:</div>
                        <div class="wire-input"><span class="wire-input-code">••••••••••••••••</span></div>
                    </div>

                    <div class="wire-action-row">
                        <div class="wire-btn wire-btn-danger wire-btn-sm">Log Out of System</div>
                        <div style="display:flex; gap:8px;">
                            <div class="wire-btn wire-btn-secondary wire-btn-sm">Cancel</div>
                            <div class="wire-btn wire-btn-success wire-btn-sm">Save Changes</div>
                        </div>
                    </div>
                </div>
            </div>

            <div class="proto-footer">
                <span class="proto-footer-brand">MBPSAAS Android Application Prototype</span>
                <span>Page 9 of 10</span>
            </div>
        </section>


        <!-- ==========================================
             PAGE 10: MOBILE ARCHITECTURE & DESIGN SYSTEM
             ========================================== -->
        <section class="prototype-page" id="p10">
            <div class="proto-header">
                <div class="proto-tag">SPECIFICATION &amp; DESIGN SYSTEM</div>
                <div class="proto-title">MOBILE ARCHITECTURE &amp; COMPOSE DESIGN SYSTEM</div>
            </div>

            <!-- Wireframe Box 1: Android Client Network Architecture -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Mobile Client Network Architecture &amp; Telemetry Pipeline
                    </div>

                    <div style="display:flex; flex-direction:column; gap:6px; font-size:0.7rem;">
                        <div style="display:flex; gap:8px; align-items:flex-start; padding:5px; background:#f4fbf5; border:1px solid var(--mbp-border-light);">
                            <span class="wire-badge wire-badge-safe" style="flex-shrink:0;">API CLIENT</span>
                            <span><strong>Retrofit Singleton (ApiClient.kt):</strong> Configured with OkHttpClient and GsonConverterFactory; dual support for Wi-Fi Mode (<code>http://10.192.10.14/mbpsaas_api/</code>) and USB Mode (<code>http://localhost:8080/mbpsaas_api/</code> via ADB reverse).</span>
                        </div>

                        <div style="display:flex; gap:8px; align-items:flex-start; padding:5px; background:#f4fbf5; border:1px solid var(--mbp-border-light);">
                            <span class="wire-badge wire-badge-safe" style="flex-shrink:0;">POLLING</span>
                            <span><strong>3-Second Coroutine Loop (HomeScreen.kt):</strong> Uses Kotlin coroutines with <code>delay(3000)</code> inside a lifecycle-aware <code>LaunchedEffect</code> to achieve pseudo-real-time poultry security telemetry.</span>
                        </div>

                        <div style="display:flex; gap:8px; align-items:flex-start; padding:5px; background:#f4fbf5; border:1px solid var(--mbp-border-light);">
                            <span class="wire-badge wire-badge-safe" style="flex-shrink:0;">STATE</span>
                            <span><strong>State Hoisting &amp; Recomposition:</strong> Mutable states for <code>events</code>, <code>zones</code>, and <code>sensors</code> drive instant reactive updates across Dashboard, Alerts, and Activity Log tabs.</span>
                        </div>

                        <div style="display:flex; gap:8px; align-items:flex-start; padding:5px; background:#f4fbf5; border:1px solid var(--mbp-border-light);">
                            <span class="wire-badge wire-badge-safe" style="flex-shrink:0;">ENDPOINTS</span>
                            <span><strong>Retrofit ApiService.kt:</strong> <code>login.php</code>, <code>forgot_password.php</code>, <code>get_motion_events.php</code>, <code>get_sensors.php</code>, <code>update_sensor_status.php</code>, and <code>update_profile.php</code>.</span>
                        </div>
                    </div>
                </div>
            </div>

            <div class="wire-arrow-down">&darr;</div>

            <!-- Wireframe Box 2: Material 3 Compose Color System -->
            <div class="wire-frame-box">
                <div class="wire-frame-inner">
                    <div class="wire-banner">
                        Uniform Material 3 Compose Color System &amp; Design Specifications
                    </div>

                    <!-- Color Swatches Grid -->
                    <div style="display:grid; grid-template-columns:repeat(6, 1fr); gap:6px; text-align:center;">
                        <div style="border:1px solid #94a3b8; padding:6px 4px; border-radius:3px; background:#ffffff;">
                            <div style="height:24px; background:#1b5e20; border-radius:2px; margin-bottom:4px;"></div>
                            <div style="font-size:0.62rem; font-weight:700; color:#102a16;">Primary Green</div>
                            <div style="font-size:0.55rem; color:var(--mbp-muted);">#1b5e20</div>
                        </div>

                        <div style="border:1px solid #94a3b8; padding:6px 4px; border-radius:3px; background:#ffffff;">
                            <div style="height:24px; background:#2e7d32; border-radius:2px; margin-bottom:4px;"></div>
                            <div style="font-size:0.62rem; font-weight:700; color:#102a16;">SafeColor</div>
                            <div style="font-size:0.55rem; color:var(--mbp-muted);">#2e7d32</div>
                        </div>

                        <div style="border:1px solid #94a3b8; padding:6px 4px; border-radius:3px; background:#ffffff;">
                            <div style="height:24px; background:#c62828; border-radius:2px; margin-bottom:4px;"></div>
                            <div style="font-size:0.62rem; font-weight:700; color:#102a16;">AlertColor</div>
                            <div style="font-size:0.55rem; color:var(--mbp-muted);">#c62828</div>
                        </div>

                        <div style="border:1px solid #94a3b8; padding:6px 4px; border-radius:3px; background:#ffffff;">
                            <div style="height:24px; background:#757575; border-radius:2px; margin-bottom:4px;"></div>
                            <div style="font-size:0.62rem; font-weight:700; color:#102a16;">DisabledColor</div>
                            <div style="font-size:0.55rem; color:var(--mbp-muted);">#757575</div>
                        </div>

                        <div style="border:1px solid #94a3b8; padding:6px 4px; border-radius:3px; background:#ffffff;">
                            <div style="height:24px; background:#0288d1; border-radius:2px; margin-bottom:4px;"></div>
                            <div style="font-size:0.62rem; font-weight:700; color:#102a16;">InfoColor</div>
                            <div style="font-size:0.55rem; color:var(--mbp-muted);">#0288d1</div>
                        </div>

                        <div style="border:1px solid #94a3b8; padding:6px 4px; border-radius:3px; background:#ffffff;">
                            <div style="height:24px; background:#102a16; border-radius:2px; margin-bottom:4px;"></div>
                            <div style="font-size:0.62rem; font-weight:700; color:#102a16;">OnSurface Ink</div>
                            <div style="font-size:0.55rem; color:var(--mbp-muted);">#102a16</div>
                        </div>
                    </div>

                    <!-- Directives -->
                    <div style="font-size:0.68rem; color:#27382b; line-height:1.45; background:#f4fbf5; padding:8px; border:1px solid var(--mbp-border-light);">
                        <strong>Mobile UI Directives:</strong>
                        All screens follow standard Android Material 3 mobile design paradigms. Interactive buttons use <code>Primary (#1b5e20)</code> or <code>SafeColor (#2e7d32)</code> with minimum 48dp touch targets. Critical intrusion alerts and warnings strictly utilize <code>AlertColor (#c62828)</code>. Normal safe conditions utilize <code>SafeColor (#2e7d32)</code>. Monospaced typography applies to all endpoint URIs, token strings, and zone codes.
                    </div>

                    <div class="wire-action-row">
                        <span style="font-size:0.65rem; color:var(--mbp-muted);">Cagayan State University &mdash; College of Information and Computing Sciences</span>
                        <div class="wire-btn wire-btn-success" onclick="window.print()">
                            🖨️ Print Complete 10-Page Mobile Prototype (PDF)
                        </div>
                    </div>
                </div>
            </div>

            <div class="proto-footer">
                <span class="proto-footer-brand">MBPSAAS Android Application Prototype</span>
                <span>Page 10 of 10</span>
            </div>
        </section>

    </main>

    <script>
        function toggleTheme() {
            const body = document.body;
            const btnText = document.getElementById('themeBtnText');
            if (body.classList.contains('color-theme')) {
                body.classList.remove('color-theme');
                body.classList.add('monochrome-theme');
                btnText.textContent = '📐 Theme: Blueprint Monochrome';
            } else {
                body.classList.remove('monochrome-theme');
                body.classList.add('color-theme');
                btnText.textContent = '🎨 Theme: Uniform App Colors';
            }
        }

        function jumpToPage(pageId) {
            const el = document.getElementById(pageId);
            if (el) {
                el.scrollIntoView({ behavior: 'smooth' });
            }
        }
    </script>
</body>
</html>
