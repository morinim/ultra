((nil
  . ((eval . (let ((root (locate-dominating-file buffer-file-name "src")))
               (load-file (expand-file-name "misc/ultra-style.el" root))))))

 (c++-mode
  . ((flycheck-checker . c/c++-clang)
     (flycheck-clang-language-standard . "c++23")
     (eval
      . (let* ((root (locate-dominating-file buffer-file-name "src"))
               (src-root (expand-file-name "src/" root)))
          (setq-local flycheck-clang-include-path
                      (list (expand-file-name "src" root)
                            (expand-file-name "src/third_party" root)))
          (setq-local flycheck-clang-args '("-stdlib=libc++"))
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
                (setq-local flycheck-clang-args
                            (append flycheck-clang-args (list "-include" header-file))))))))))

 (c++-ts-mode
  . ((flycheck-checker . c/c++-clang)
     (flycheck-clang-language-standard . "c++23")
     (eval
      . (let* ((root (locate-dominating-file buffer-file-name "src"))
               (src-root (expand-file-name "src/" root)))
          (setq-local flycheck-clang-include-path
                      (list (expand-file-name "src" root)
                            (expand-file-name "src/third_party" root)))
          (setq-local flycheck-clang-args '("-stdlib=libc++"))
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
                (setq-local flycheck-clang-args
                            (append flycheck-clang-args (list "-include" header-file)))))))))))
