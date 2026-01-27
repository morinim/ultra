;;; ultra-style.el --- Ultra's C/C++/Python style

;; Copyright (C) 2024 EOS di Manlio Morini.
;;
;; This Source Code Form is subject to the terms of the Mozilla Public
;; License, v. 2.0. If a copy of the MPL was not distributed with this file,
;; You can obtain one at http://mozilla.org/MPL/2.0/

;; ----------------------------------------------------------------------------
;; C++
;; ----------------------------------------------------------------------------
(add-to-list 'auto-mode-alist '("\\.h\\'" . c++-mode))
(add-to-list 'auto-mode-alist '("\\.tcc\\'" . c++-mode))

(c-add-style "ultra-style"
  '("bsd"
   (c-basic-offset . 2)                        ; indent by two spaces
   (indent-tabs-mode . nil)                    ; use spaces rather than tabs
   (c-offsets-alist . ((innamespace . [0]))))) ; suppress namespace indentation

(defun ultra-c++-mode-hook ()
  (c-set-style "ultra-style"))        ; use my-style defined above

(add-hook 'c++-mode-hook #'ultra-c++-mode-hook)

(add-hook 'c-mode-common-hook
          (lambda ()
            (add-hook 'before-save-hook
                      #'delete-trailing-whitespace nil t)))

(defface ultra-contract-wrapper-face
  '((t :inherit font-lock-comment-face))
  "Face for contract wrappers like assert, Expects, Ensures.")

(defface ultra-contract-body-face
  '((t :inherit shadow))
  "Face for the body of contract expressions.")

(defun ultra-contract-matcher (limit)
  "Font-lock matcher for contract expressions with balanced parentheses."
  (when (re-search-forward
         (rx symbol-start (or "assert" "Expects" "Ensures") "(")
         limit t)
    (let* ((kw-start (match-beginning 0))
           (open-paren (1- (point)))
           (close-paren (ignore-errors (scan-sexps open-paren 1))))
      (when close-paren
        ;; Match 0: whole wrapper (keyword + parentheses)
        ;; Match 1: body inside parentheses
        (set-match-data
         (list
          kw-start close-paren          ; 0: whole wrapper
          (1+ open-paren) (1- close-paren))) ; 1: body only
        t))))

(defconst ultra-contract-font-lock-keywords
  '((ultra-contract-matcher
     (0 'ultra-contract-wrapper-face t)
     (1 'ultra-contract-body-face t))))

(add-hook 'c-mode-common-hook
          (lambda ()
            (font-lock-add-keywords
             nil ultra-contract-font-lock-keywords 'append)
            (font-lock-flush)))

;; ----------------------------------------------------------------------------
;; Python
;; ----------------------------------------------------------------------------
(setq-default python-indent-offset 4)
(setq-default tab-width 4)
(add-hook 'python-mode-hook
          (lambda ()
            (add-hook 'before-save-hook
                      #'delete-trailing-whitespace nil t)))


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
