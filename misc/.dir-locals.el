((nil
  . ((eval . (let ((root (locate-dominating-file
                           buffer-file-name "misc/ultra-style.el")))
               (load-file (expand-file-name "misc/ultra-style.el" root))))))

 (c++-mode
  . ((flycheck-clang-language-standard . "c++23")
     (flycheck-gcc-language-standard . "c++23")
     (eval
      . (let* ((root (locate-dominating-file
                      buffer-file-name "misc/ultra-style.el"))
               (src (expand-file-name "src" root))
               (third-party (expand-file-name "src/third_party" root))
               (src-root (file-name-as-directory src)))
          (setq-local flycheck-checker
                      (cond
                       ((executable-find "clang++") 'c/c++-clang)
                       ((executable-find "g++")     'c/c++-gcc)
                       (t nil)))

          (setq-local flycheck-clang-include-path
                      (list src third-party))
          (setq-local flycheck-gcc-include-path
                      (list src third-party))
          (when (eq flycheck-checker 'c/c++-clang)
            (setq-local flycheck-clang-args '("-stdlib=libc++")))
          (when (and buffer-file-name (string-match-p "\\.tcc\\'" buffer-file-name))
            (let* ((relative-file (file-relative-name buffer-file-name src-root))
                   (header-map
                    '(("kernel/layered_population_iterator.tcc" . "kernel/layered_population.h")
                      ("kernel/gp/individual_exon_view.tcc" . "kernel/gp/individual.h")
                      ("kernel/gp/individual_iterator.tcc" . "kernel/gp/individual.h")
                      ("kernel/gp/src/oracle_internal.tcc" . "kernel/gp/src/oracle.h")
                      ("kernel/gp/src/evaluator_internal.tcc" . "kernel/gp/src/evaluator.h")))
                   (mapped-header (cdr (assoc relative-file header-map)))
                   (header-file (if mapped-header
                                    (expand-file-name mapped-header src-root)
                                  (replace-regexp-in-string "\\.tcc\\'" ".h" buffer-file-name))))
              (when (file-exists-p header-file)
                (cond
                 ((eq flycheck-checker 'c/c++-clang)
                  (setq-local flycheck-clang-args
                              (append (bound-and-true-p flycheck-clang-args)
                                      (list "-include" header-file))))
                 ((eq flycheck-checker 'c/c++-gcc)
                  (setq-local flycheck-gcc-args
                              (append (bound-and-true-p flycheck-gcc-args)
                                      (list "-include" header-file))))))))))))

 (c++-ts-mode
  . ((flycheck-clang-language-standard . "c++23")
     (flycheck-gcc-language-standard . "c++23")
     (eval
      . (let* ((root (locate-dominating-file
                      buffer-file-name "misc/ultra-style.el"))
               (src (expand-file-name "src" root))
               (third-party (expand-file-name "src/third_party" root))
               (src-root (file-name-as-directory src)))
          (setq-local flycheck-checker
                      (cond
                       ((executable-find "clang++") 'c/c++-clang)
                       ((executable-find "g++")     'c/c++-gcc)
                       (t nil)))

          (setq-local flycheck-clang-include-path
                      (list src third-party))
          (setq-local flycheck-gcc-include-path
                      (list src third-party))
          (when (eq flycheck-checker 'c/c++-clang)
            (setq-local flycheck-clang-args '("-stdlib=libc++")))
          (when (and buffer-file-name (string-match-p "\\.tcc\\'" buffer-file-name))
            (let* ((relative-file (file-relative-name buffer-file-name src-root))
                   (header-map
                    '(("kernel/layered_population_iterator.tcc" . "kernel/layered_population.h")
                      ("kernel/gp/individual_exon_view.tcc" . "kernel/gp/individual.h")
                      ("kernel/gp/individual_iterator.tcc" . "kernel/gp/individual.h")
                      ("kernel/gp/src/oracle_internal.tcc" . "kernel/gp/src/oracle.h")
                      ("kernel/gp/src/evaluator_internal.tcc" . "kernel/gp/src/evaluator.h")))
                   (mapped-header (cdr (assoc relative-file header-map)))
                   (header-file (if mapped-header
                                    (expand-file-name mapped-header src-root)
                                  (replace-regexp-in-string "\\.tcc\\'" ".h" buffer-file-name))))
              (when (file-exists-p header-file)
                (cond
                 ((eq flycheck-checker 'c/c++-clang)
                  (setq-local flycheck-clang-args
                              (append (bound-and-true-p flycheck-clang-args)
                                      (list "-include" header-file))))
                 ((eq flycheck-checker 'c/c++-gcc)
                  (setq-local flycheck-gcc-args
                              (append (bound-and-true-p flycheck-gcc-args)
                                      (list "-include" header-file)))))))))))))
