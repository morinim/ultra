((nil
  . ((eval . (let ((root (locate-dominating-file buffer-file-name "src")))
               (load-file (expand-file-name "misc/ultra-style.el" root))))))

 (c++-mode
  . ((flycheck-checker . c/c++-clang)
     (flycheck-clang-language-standard . "c++23")
     (eval
      . (let ((root (locate-dominating-file buffer-file-name "src")))
          (setq-local flycheck-clang-include-path
                      (list (expand-file-name "src" root)))))))

 (c++-ts-mode
  . ((flycheck-checker . c/c++-clang)
     (flycheck-clang-language-standard . "c++23")
     (eval
      . (let ((root (locate-dominating-file buffer-file-name "src")))
          (setq-local flycheck-clang-include-path
                      (list (expand-file-name "src" root))))))))
