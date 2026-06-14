;;; ultra-style.el --- Ultra's C/C++/Python/Prolog style

;; Copyright (C) 2024 EOS di Manlio Morini.
;;
;; This Source Code Form is subject to the terms of the Mozilla Public
;; License, v. 2.0. If a copy of the MPL was not distributed with this file,
;; You can obtain one at http://mozilla.org/MPL/2.0/

;; ----------------------------------------------------------------------------
;; C++
;; ----------------------------------------------------------------------------
(require 'cc-mode)

(add-to-list 'auto-mode-alist '("\\.h\\'" . c++-mode))
(add-to-list 'auto-mode-alist '("\\.tcc\\'" . c++-mode))

(c-add-style "ultra-style"
  '("bsd"
   (c-basic-offset . 2)                      ; indent by two spaces
   (indent-tabs-mode . nil)                  ; use spaces rather than tabs
   (c-offsets-alist . ((innamespace . 0))))) ; suppress namespace indentation

(defun ultra-c++-mode-hook ()
  "Apply Ultra's C++ indentation style."
  (c-set-style "ultra-style"))

(add-hook 'c++-mode-hook #'ultra-c++-mode-hook)

(add-hook 'prog-mode-hook
          (lambda ()
            (add-hook 'before-save-hook
                      #'delete-trailing-whitespace nil t)))

(defface ultra-contract-wrapper-face
  '((t :inherit font-lock-comment-face))
  "Face for contract wrappers like assert, Expects, Ensures.")

(defface ultra-contract-body-face
  '((t :inherit shadow))
  "Face for the body of contract expressions.")

(defconst ultra-contract-regexp
  (rx symbol-start (or "assert" "Expects" "Ensures") "(")
  "Regexp matching the start of a contract expression.")

(defvar font-lock-beg)
(defvar font-lock-end)

(defun ultra-contract-in-comment-or-string-p (pos)
  "Return non-nil if POS is inside a comment or string."
  (save-excursion
    (nth 8 (syntax-ppss pos))))

(defun ultra-contract-keyword-start (open-paren)
  "Return contract keyword start before OPEN-PAREN, or nil."
  (save-excursion
    (goto-char open-paren)
    (skip-syntax-backward "w_")
    (when (and (member (buffer-substring-no-properties (point) open-paren)
                       '("assert" "Expects" "Ensures"))
               (or (= (point) (point-min))
                   (not (memq (char-syntax (char-before))
                              '(?w ?_)))))
      (point))))

(defun ultra-contract-bounds-around (pos)
  "Return contract expression bounds around POS, or nil."
  (save-excursion
    (catch 'bounds
      (let (open-paren)
        (while (setq open-paren (nth 1 (syntax-ppss pos)))
          (let* ((kw-start (ultra-contract-keyword-start open-paren))
                 (close-paren (and kw-start
                                   (ignore-errors
                                     (scan-sexps open-paren 1)))))
            (when (and close-paren (< pos close-paren))
              (throw 'bounds (cons kw-start close-paren)))
            (setq pos open-paren)))))))

(defun ultra-contract-extend-region ()
  "Extend font-lock region to cover multiline contract expressions."
  (let ((changed nil))
    (dolist (pos (list font-lock-beg font-lock-end))
      (let ((bounds (ultra-contract-bounds-around pos)))
        (when bounds
          (when (< (car bounds) font-lock-beg)
            (setq font-lock-beg (car bounds)
                  changed t))
          (when (> (cdr bounds) font-lock-end)
            (setq font-lock-end (cdr bounds)
                  changed t)))))

    (save-excursion
      (goto-char font-lock-beg)
      (while (re-search-forward ultra-contract-regexp font-lock-end t)
        (unless (ultra-contract-in-comment-or-string-p (match-beginning 0))
          (let* ((kw-start (match-beginning 0))
                 (open-paren (1- (match-end 0)))
                 (close-paren (ignore-errors (scan-sexps open-paren 1))))
            (when close-paren
              (when (< kw-start font-lock-beg)
                (setq font-lock-beg kw-start
                      changed t))
              (when (> close-paren font-lock-end)
                (setq font-lock-end close-paren
                      changed t)))))))

    changed))

(defun ultra-contract-matcher (limit)
  "Font-lock matcher for contract expressions with balanced parentheses."
  (catch 'matched
    (while (re-search-forward ultra-contract-regexp limit t)
      (unless (ultra-contract-in-comment-or-string-p (match-beginning 0))
        (let* ((kw-start (match-beginning 0))
               (open-paren (1- (point)))
               (close-paren (ignore-errors (scan-sexps open-paren 1))))
          (when (and close-paren (<= close-paren limit))
            (set-match-data
             (list
              kw-start close-paren
              (1+ open-paren) (1- close-paren)))
            (throw 'matched t)))))))

(defconst ultra-contract-font-lock-keywords
  '((ultra-contract-matcher
     (0 'ultra-contract-wrapper-face t)
     (1 'ultra-contract-body-face t)
     (0 (progn
          (put-text-property (match-beginning 0)
                             (match-end 0)
                             'font-lock-multiline
                             t)
          nil)))))

(add-hook 'c-mode-common-hook
          (lambda ()
            (font-lock-add-keywords
             nil ultra-contract-font-lock-keywords 'append)
            (add-hook 'font-lock-extend-region-functions
                      #'ultra-contract-extend-region nil t)))


;; ----------------------------------------------------------------------------
;; Python
;; ----------------------------------------------------------------------------
(setq-default python-indent-offset 4)
(setq-default tab-width 4)


;; ----------------------------------------------------------------------------
;; Prolog
;; ----------------------------------------------------------------------------
(add-to-list 'auto-mode-alist '("\\.\\(pl\\|pro\\|lgt\\)\\'" . prolog-mode))


;; ----------------------------------------------------------------------------
;; Mix
;; ----------------------------------------------------------------------------
(setq-default indent-tabs-mode nil)  ;; indent using spaces instead of tabs

(setq-default show-trailing-whitespace t)
(setq-default indicate-empty-lines t)
(column-number-mode 1)

(show-paren-mode 1)

(provide 'ultra-style)
;;; ultra-style.el ends here
